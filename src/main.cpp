#include "sdk.h"

#include <boost/program_options.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>

#include <iostream>
#include <filesystem>
#include <thread>

#include "loader/json_loader.h"
#include "loader/extra_data.h"
#include "handler/request_handler.h"
#include "handler/handler_api.h"
#include "util/ticker.h"
#include "logger/logger.h"
#include "./model/model_serialization.h"
#include "./repository/postgres.h"

namespace net = boost::asio;
namespace sys = boost::system;
namespace fs = std::filesystem;

namespace {

using namespace std::literals;

struct Args {
    size_t      tick_period;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points = false;
    bool is_tick_period = false;
    std::string state_file_path;
    bool has_state_file_path;
    size_t save_state_period;
    bool has_save_state_period;    
}; 

// Запускает функцию fn на num_threads потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned num_threads, const Fn& fn) {
    num_threads = std::max(1u, num_threads);
    std::vector<std::thread> workers;
    workers.reserve(num_threads - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--num_threads) {
        workers.emplace_back(fn);
    }
    fn();
    for (auto i = 0; i < workers.size(); i++) {
        workers[i].join();
    }
}

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_file)->value_name("file"s), "Destination file name")
        ("www-root,w", po::value(&args.www_root)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions")
        ("state-file,s", po::value(&args.state_file_path)->value_name("file"s), "set game state file path")
        ("save-state-period,p", po::value<size_t>(&args.save_state_period)->value_name("milliseconds"s), "set game state save period");

    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
        return std::nullopt;
    }

    // Проверяем наличие опций src и dst
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Source maps path is not specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Source static files path is not specified"s);
    }

    args.randomize_spawn_points = vm.contains("randomize-spawn-points"s);
    args.is_tick_period = vm.contains("tick-period"s);
    args.has_state_file_path = vm.contains("state-file");
    args.has_save_state_period = vm.contains("save-state-period");
    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}

constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};

std::string GetDbURLFromEnv() {
    if (const auto* url = std::getenv(DB_URL_ENV_NAME)) {
        return std::string{url};
    } else {
        throw std::runtime_error(DB_URL_ENV_NAME + " environment variable not found"s);
    }
    return {};
}

}  // namespace

int main(int argc, const char* argv[]) {
    // 0. Инициализация логгера
    Logger::InitBoostLogFilter();

    try {
        auto arguments = ParseCommandLine(argc, argv);
        if (!arguments) {
            return EXIT_SUCCESS;    
        }

        auto args = arguments.value();

        // 1. Загружаем карту из файла и построить модель игры
        auto [game, extra_data] = json_loader::LoadGame(args.config_file);
        game.SetRandomSpawn(args.randomize_spawn_points);

        // 1.1 Создаем БД postgres
        postgres::DatabasePostgres db{/*std::thread::hardware_concurrency()*/1,GetDbURLFromEnv()};

        // 1.2 Создаем сервис игры
        service::Service service(game, db);

        // 1.3 загружаем сохраненное состояние игры
        serialization::ServiceSerializator serializator(service, game, args.state_file_path, args.has_state_file_path);
        serializator.Restore();        

        // 1.4 Добавляем обработчик автосохранения
        if (args.has_save_state_period) {
            auto save_period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<size_t, std::milli>(args.save_state_period));
            serializator.AutoSave(save_period);
        }

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                Logger::LogStop(0);
                ioc.stop();
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        // Обработчик API
        http_handler::ApiHandler api_handler(service, extra_data);
        // strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);

        // Настраиваем вызов метода Service::Tick каждые 50 миллисекунд внутри strand
        if (args.is_tick_period){
            service.OnTimeTicker();
            auto ticker = std::make_shared<Ticker>(api_strand, std::chrono::milliseconds{args.tick_period},
                [&service](std::chrono::milliseconds delta) { service.Tick(delta); }
            );
            ticker->Start();
        }

        fs::path static_files_root{args.www_root};
        // Создаём обработчик запросов в куче, управляемый shared_ptr
        auto handler = std::make_shared<http_handler::RequestHandler>(api_handler, api_strand, std::move(static_files_root));
        // Оборачиваем его в логирующий декоратор
        Logger::LoggingRequestHandler logging_handler(
            [handler](auto&& endpoint, auto&& req, auto&& send) {
                // Обрабатываем запрос
                (*handler)(
                    std::forward<decltype(req)>(req),
                    std::forward<decltype(send)>(send));
                }
        );

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, logging_handler);

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        Logger::LogStart(address, port);

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        // 7. Сериализуем данные
        serializator.Serialize();        
    } catch (const std::exception& ex) {
        Logger::LogStop(ex);
        return EXIT_FAILURE;
    }
}

#pragma once
#include <ostream>
#include <iostream>

#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильт
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/date_time.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>

#include <boost/system.hpp>
#include <boost/asio/buffer.hpp>

#include <boost/json.hpp>
#include <boost/describe.hpp>

#include <string_view>

namespace Logger {

using namespace std::literals;

namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace net = boost::asio;
namespace json = boost::json;

struct LogMsg {
    LogMsg() = delete;
    static constexpr std::string_view SERVER_START  = "server started"sv;
    static constexpr std::string_view SERVER_STOP   = "server exited"sv;
    static constexpr std::string_view REQ_RECEIVED  = "request received"sv;
    static constexpr std::string_view RESP_SENT     = "response sent"sv;
    static constexpr std::string_view ERROR         = "error"sv;
};


void InitBoostLogFilter();

template <class T>
struct LogMessage{
    explicit LogMessage(T&& custom_data):
        data(custom_data){
    }
    
    T data;
};

template <class T>
std::string CreateLogMessage(T&& data){
    return json::serialize(boost::json::value_from(data));
}


void info(std::string_view data_, std::string_view message_);

void LogStart(const net::ip::address& address, net::ip::port_type port);
void LogError(beast::error_code ec, std::string_view what);
void LogStop(int code);
void LogStop(const std::exception& ex);

struct ServerAddressLogData {
    ServerAddressLogData(std::string addr, uint32_t prt): 
        address(addr), port(prt) {};

    std::string address;
    uint32_t port;
};
BOOST_DESCRIBE_STRUCT(ServerAddressLogData, (),(address,port) )

struct RequestLogData {
    RequestLogData(std::string ip_addr, std::string url, std::string method):
            ip(ip_addr),
            URI(url),
            method(method) {};

    std::string ip;
    std::string URI;
    std::string method;
};
BOOST_DESCRIBE_STRUCT(RequestLogData, (), (ip,URI,method) )

struct ResponseLogData {
    ResponseLogData(std::string ip_addr, uint64_t res_time, int code, std::string content_type):
            ip(ip_addr),
            response_time(res_time),
            code(code),
            content_type(content_type) {};

    std::string ip;
    uint64_t response_time;
    int code;
    std::string content_type;
};
BOOST_DESCRIBE_STRUCT(ResponseLogData, (), (ip,response_time,code,content_type) )


struct ExceptionLogData {
    ExceptionLogData(int code, std::string_view text, std::string_view where): 
        code(code), text(text), where(where) {}

    uint32_t code;
    std::string_view text;
    std::string_view where;
};

BOOST_DESCRIBE_STRUCT(ExceptionLogData, (),
    (code,text,where) 
)

struct ServerStopLogData {
    ServerStopLogData(int code, std::string exception =""): 
        code(code), exception(exception) {};

    uint32_t code;
    std::string exception;
};
BOOST_DESCRIBE_STRUCT(ServerStopLogData, (), (code,exception) )

template<class RequestHandler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(RequestHandler&& handler)
        : decorated_{handler} {}

    template <typename Body, typename Allocator, typename Send>
    void operator() (const net::ip::tcp::endpoint& endpoint, http::request<Body, http::basic_fields<Allocator>>&& req,
         Send&& send) {
        LogRequest(req, endpoint);
        auto start = std::chrono::steady_clock::now();
        decorated_(endpoint, std::move(req), [send, start, &endpoint](auto&& response){
            LogResponse(response, start, endpoint);
            send(response);
        });
    }

private:
    template <typename Body, typename Allocator>
    static void LogRequest(const http::request<Body, http::basic_fields<Allocator>>& req, const net::ip::tcp::endpoint& endpoint) {
        info(CreateLogMessage(RequestLogData(
                        endpoint.address().to_string(),
                        req.target(),
                        req.method_string())),
                        LogMsg::REQ_RECEIVED
                    );
     }

    template <typename Body>
    static void LogResponse(http::response<Body>& response, std::chrono::steady_clock::time_point s, const net::ip::tcp::endpoint& endpoint) {
        info(CreateLogMessage(ResponseLogData(
                        endpoint.address().to_string(),
                        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - s).count(),
                        response.result_int(),
                        response[http::field::content_type])),
                        LogMsg::RESP_SENT
                    );
    }

private:
    RequestHandler decorated_;
};

}
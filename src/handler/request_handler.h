#pragma once

#include "../http/http_server.h"
#include "../model/model.h"
#include "../loader/json_loader.h"

#include "handler_api.h"
#include "response.h"
#include "request.h"

#include <boost/asio/strand.hpp>

#include <filesystem>
#include <iostream>
#include <variant>


namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;


namespace fs = std::filesystem;
using namespace std::literals;


// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;


class RequestHandler : public std::enable_shared_from_this<RequestHandler>{
public:
    using Strand = net::strand<net::io_context::executor_type>;

    typedef void (Handler) (StringRequest& request);

    explicit RequestHandler(ApiHandler& api_handler, Strand api_strand, fs::path basePath) 
            : api_handler_{api_handler}
            , api_strand_{api_strand}
            , rootPath_{std::move(fs::weakly_canonical(basePath))} { }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        try {
            HandleRequest(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        } catch (...) {         
            http_request::RequestData data(req);
            send(ErrorBuilder::MakeErrorResponse(ErrorBuilder::ErrorCode::ServerError, data));
        }
    }

private:

    template <typename Body, typename Allocator, typename Send>
    void HandleRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        // Обработать запрос request и отправить ответ, используя send
        if (IsApiRequest(req)) {
            auto handle = [self = shared_from_this(), send, req = std::forward<decltype(req)>(req)]() {
                try {
                    send(self->api_handler_.HandleRequest(req));
                } catch (...) {
                    http_request::RequestData data(req);
                    send(ErrorBuilder::MakeErrorResponse(ErrorBuilder::ErrorCode::ServerError, data));
                }
            };
            return net::dispatch(api_strand_, handle);
        }
        return std::visit(
            [&send](auto&& result) {
                send(std::forward<decltype(result)>(result));
            },
            MakeFileResponse(std::forward<decltype(req)>(req),rootPath_)
        );
    }



    template <typename Body, typename Allocator>
    bool IsApiRequest(const http::request<Body, http::basic_fields<Allocator>>& req) const {
        auto decoded_uri = util::DecodeURI(req.target());
        if (!decoded_uri.has_value()) {
            return false;
        }
        fs::path uri(decoded_uri.value());
        return util::IsSubPath(uri, http_handler::ApiTokens::api_root);
    }

private:
    fs::path rootPath_;
    Strand api_strand_;
    ApiHandler& api_handler_;
};


}  // namespace http_handler

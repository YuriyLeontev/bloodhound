#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <filesystem>
#include <optional>
#include <string_view>
#include <unordered_map>

#include "response_messages.h"
#include "request.h"
#include "../util/util.h" 

namespace http_request {


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;
namespace fs = std::filesystem;
namespace sys = boost::system;

using namespace std::literals;

using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;

struct ConstantsResponse {
    ConstantsResponse() = delete;
    static constexpr std::string_view NO_CACHE      = "no-cache"sv;
    static constexpr std::string_view INDEX_HTML    = "index.html"sv;
    static constexpr std::string_view EMPTY_JSON    = "{}"sv;
};

StringResponse MakeStringResponse(http::status status, std::string_view body, const http_request::RequestData& req_data,
                                  std::string_view content_type);


class ErrorBuilder {
    ErrorBuilder() = delete;
    
public:

    enum class ErrorCode {
        Ok,
        BadRequest,
        BadContentType,
        MapNotFound,
        FileNotFound,
        InvalidMethod,
        InvalidURI,
        ServerError,
        JoinGameParse,
        InvalidAuthHeader,
        PlayerTokenNotFound,
        ActionParse,
        TickParse,
        TickError,
    };

    static StringResponse MakeErrorResponse(ErrorCode ec, const http_request::RequestData& req_data, std::optional<std::string_view> param = std::nullopt) {
        auto [status, body, content_type] = Error(ec, param);
        return MakeStringResponse(status, body, req_data, content_type);
    }

private:
    static std::string SerializeError(std::string_view code, std::string_view msg) {
        static constexpr std::string_view code_key = "code"sv;
        static constexpr std::string_view msg_key = "message"sv;
        json::object obj;
        obj.emplace(code_key, code);
        obj.emplace(msg_key, msg);
        return json::serialize(obj);
    }

    static std::tuple<http::status, std::string, std::string_view> Error(ErrorCode ec, std::optional<std::string_view> param) {
        switch (ec) {
        case ErrorCode::MapNotFound:
            return {http::status::not_found, SerializeError(Codes::MapNotFound, Messages::MapNotFound), ContentType::APPLICATION_JSON};
        case ErrorCode::BadRequest:
            return {http::status::bad_request, SerializeError(Codes::BadRequest, Messages::BadRequest), ContentType::APPLICATION_JSON};
        case ErrorCode::BadContentType:
            return {http::status::bad_request, SerializeError(Codes::InvalidArgument, Messages::BadContentType), ContentType::APPLICATION_JSON};            
        case ErrorCode::InvalidURI:
            return {http::status::bad_request, SerializeError(Codes::BadRequest, Messages::InvalidURI), ContentType::APPLICATION_JSON};
        case ErrorCode::FileNotFound:
            return {http::status::not_found, std::string{Messages::FileNotFound}, ContentType::TEXT_PLAIN};            
        case ErrorCode::JoinGameParse:
            return {http::status::bad_request, SerializeError(Codes::InvalidArgument, Messages::JoinGameParse), ContentType::APPLICATION_JSON};
        case ErrorCode::InvalidAuthHeader:
            return {http::status::unauthorized, SerializeError(Codes::InvalidAuth, Messages::InvalidAuth), ContentType::APPLICATION_JSON};
        case ErrorCode::PlayerTokenNotFound:
            return {http::status::unauthorized, SerializeError(Codes::InvalidToken, Messages::InvalidToken), ContentType::APPLICATION_JSON};
        case ErrorCode::ActionParse:
            return {http::status::bad_request, SerializeError(Codes::InvalidArgument, Messages::ActionParse), ContentType::APPLICATION_JSON};
        case ErrorCode::TickParse:
            return {http::status::bad_request, SerializeError(Codes::InvalidArgument, Messages::TickParse), ContentType::APPLICATION_JSON};            
        case ErrorCode::TickError:
            return {http::status::bad_request, SerializeError(Codes::BadRequest, Messages::TickError), ContentType::APPLICATION_JSON};            
        case ErrorCode::InvalidMethod: {
            std::ostringstream message;
            message << Messages::InvalidMethod;
            if (param.has_value()) {
                message << ". Expected methods: "sv << param.value();
            }
            return {http::status::method_not_allowed, SerializeError(Codes::InvalidMethod, message.str()), ContentType::APPLICATION_JSON};
        }
        default:
            assert(false);
            return {};
        }
    }
};

template <typename Body, typename Allocator>
std::variant<StringResponse, FileResponse> MakeFileResponse( http::request<Body, http::basic_fields<Allocator>>&& req, const fs::path& rootPath){
    http_request::RequestData data(req);

    if (data.method != http::verb::get) {
        auto response = ErrorBuilder::MakeErrorResponse(ErrorBuilder::ErrorCode::InvalidMethod, data);
        response.set(http::field::allow, Methods::GET);
        return response;
    }

    http::response<http::file_body> response;
    response.version(11);  // HTTP/1.1
    response.result(http::status::ok);

    auto target = util::DecodeURI(req.target());
    if (!target){
        return ErrorBuilder::MakeErrorResponse(ErrorBuilder::ErrorCode::BadRequest, data);
    }
    std::string_view decoded_uri = target.value();

    fs::path static_content{rootPath};
    if (!util::IsSubPath(static_content,rootPath)){
        return ErrorBuilder::MakeErrorResponse(ErrorBuilder::ErrorCode::BadRequest, data);
    }

    fs::path default_path{ConstantsResponse::INDEX_HTML};
    if(decoded_uri.empty() || decoded_uri == "/") {
        static_content = fs::weakly_canonical(static_content / default_path);
    } else {
        std::string_view pathStr = decoded_uri.substr(1, decoded_uri.size() - 1);
        fs::path rel_path{pathStr};
        static_content = fs::weakly_canonical(static_content / rel_path);

        if(fs::is_directory(static_content)) {
            static_content = fs::weakly_canonical(static_content / default_path);
        }        
    }

    if (!fs::exists(static_content)){
        StringResponse error = ErrorBuilder::MakeErrorResponse(ErrorBuilder::ErrorCode::FileNotFound, data);
        error.body().append(data.decoded_uri.value());
        error.content_length(error.body().size());
        return error;
    }

    std::string_view content = ContentType::FromFileExt(
                boost::algorithm::to_lower_copy(static_content.extension().string())
            );

    response.insert(http::field::content_type, content);
    
    http::file_body::value_type file;

    if (sys::error_code ec; file.open(static_content.c_str(), beast::file_mode::read, ec), ec) {
        StringResponse error = ErrorBuilder::MakeErrorResponse(ErrorBuilder::ErrorCode::FileNotFound, data);
        error.body().append(data.decoded_uri.value());
        error.content_length(error.body().size());
        return error;
    } else {
        response.body() = std::move(file);
    }

    // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
    // в зависимости от свойств тела сообщения
    response.prepare_payload();

    return response;
};

}
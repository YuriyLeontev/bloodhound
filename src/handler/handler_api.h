#pragma once

#include "../service/service.h"
#include "../util/util.h"
#include "../loader/json_loader.h"
#include "../loader/extra_data.h"

#include "handler_constants.h"
#include "response.h"

#include <vector>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <filesystem>

namespace http_handler{

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace fs = std::filesystem;

using namespace std::literals;
using namespace http_request;

using ErrorCode = http_request::ErrorBuilder::ErrorCode;
using StringResponse = http::response<http::string_body>;

class ApiHandler {

public:
    explicit ApiHandler(service::Service& service, const extra_data::ExtraData& extra_data);

    template <typename Body, typename Allocator>
    StringResponse  HandleRequest( const http::request<Body, http::basic_fields<Allocator>>& req) {

        req_data_.SetData(req);

        if (!req_data_.decoded_uri.has_value()) {
            return ResponseApiError(ErrorBuilder::ErrorCode::InvalidURI);
        }
        req_tokens_ = util::SplitIntoTokens(*req_data_.decoded_uri, '/');
        if (req_tokens_.empty() || req_tokens_.front() != ApiTokens::API) {
            return ResponseApiError(ErrorBuilder::ErrorCode::BadRequest);
        }
        req_tokens_.pop();
        if (req_tokens_.empty()) {
            return ResponseApiError(ErrorBuilder::ErrorCode::BadRequest);
        }
        auto version = req_tokens_.front(); req_tokens_.pop();
        
        if (version != ApiTokens::V1) {
            return ResponseApiError(ErrorBuilder::ErrorCode::BadRequest);
        }
        if (req_tokens_.empty()) {
            return ResponseApiError(ErrorBuilder::ErrorCode::BadRequest);
        }
        auto token = req_tokens_.front(); req_tokens_.pop();
        if (token == ApiTokens::MAPS) {
            return HandleMapsRequest(version);
        }
        if (token == ApiTokens::GAME) {
            return HandleGameRequest(version);
        }        
        return ResponseApiError(ErrorBuilder::ErrorCode::BadRequest);
    }

private:
    StringResponse ResponseApiError(ErrorBuilder::ErrorCode ec) const;

    StringResponse HandleMapsRequest(std::string_view version) const;
    StringResponse HandleAllMapsRequest(std::string_view version) const;
    StringResponse HandleSingleMapRequest(std::string_view version) const;

    StringResponse HandleGameRequest(std::string_view version) const;
    StringResponse HandlePlayerJoin(std::string_view version) const;
    StringResponse HandlePlayersRequest(std::string_view version) const;
    StringResponse HandleStateRequest(std::string_view version) const;
    StringResponse HandlePlayerActionRequest(std::string_view version) const;
    
    StringResponse HandleTickRequest(std::string_view version) const;
    
    StringResponse HandleRecordsRequest(std::string_view api_token, std::string_view version) const;


    template <typename... Args>
    StringResponse MakeInvalidMethodResponse(const Args&... args) const {
        std::ostringstream methods;
        PrintMethods(methods, args...);
        auto response = ErrorBuilder::MakeErrorResponse(ErrorBuilder::ErrorCode::InvalidMethod, req_data_, methods.str());
        response.set(http::field::allow, methods.str());
        return response;
    }     

    template <typename Arg, typename... Args>
    bool IsMethodOneOfAllowed(const Arg& arg, const Args&... args) const {
        if (req_data_.method == arg)
            return true;
        if constexpr (sizeof...(args) != 0) {
            return IsMethodOneOfAllowed(args...);
        }
        return false;
    }

    template <typename Fn, typename... Args>
    StringResponse ExecuteAllowedMethods(Fn&& action, const Args&... allowed_methods) const {
        if (!IsMethodOneOfAllowed(allowed_methods...)) {
            return MakeInvalidMethodResponse(allowed_methods...);
        }
        return action();
    }

    template <typename Arg, typename... Args>
    void PrintMethods(std::ostream& out, const Arg& arg, const Args&... args) const {
        out << Methods::method_to_str.at(arg);
        if constexpr (sizeof...(args) != 0) {
            out << ", "sv;
            PrintMethods(out, args...);
        }
    } 

    template <typename Fn>
    StringResponse ExecuteAuthorized(Fn&& action) const {
        if (!req_data_.auth_token.has_value()) {
            return ResponseApiError(ErrorCode::InvalidAuthHeader);
        }
        return action(req_data_.auth_token.value());
    }    

private:
    service::Service & service_;
    const extra_data::ExtraData& extra_data_;

    http_request::RequestData req_data_;
    mutable std::queue<std::string_view> req_tokens_;
};

}

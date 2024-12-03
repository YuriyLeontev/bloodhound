#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <optional>
#include <regex>

#include "../service/service.h"
#include "../util/util.h" 

namespace http_request {

namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

struct Methods {
    Methods() = delete;
    static constexpr std::string_view GET = "GET"sv;
    static constexpr std::string_view HEAD = "HEAD"sv;
    static constexpr std::string_view POST = "POST"sv;
    static const std::unordered_map<http::verb, std::string_view> method_to_str;
};

struct ContentType {
    ContentType() = delete;
    static constexpr std::string_view TEXT_HTML         = "text/html"sv;
    static constexpr std::string_view TEXT_CSS          = "text/html"sv;
    static constexpr std::string_view TEXT_PLAIN        = "text/plain"sv;
    static constexpr std::string_view TEXT_JS           = "text/javascript"sv;
    static constexpr std::string_view APPLICATION_JSON  = "application/json"sv;
    static constexpr std::string_view APPLICATION_XML   = "text/xml"sv;
    static constexpr std::string_view IMAGE_PNG         = "image/png"sv;
    static constexpr std::string_view IMAGE_JPG         = "image/jpeg"sv;
    static constexpr std::string_view IMAGE_GIF         = "image/gif"sv;
    static constexpr std::string_view IMAGE_BMP         = "image/bmp"sv;
    static constexpr std::string_view IMAGE_ICO         = "image/vnd.microsoft.icon"sv;
    static constexpr std::string_view IMAGE_TIFF        = "image/tiff"sv;
    static constexpr std::string_view IMAGE_SVG         = "image/svg+xml"sv;
    static constexpr std::string_view AUDIO_MP3         = "audio/mpeg"sv;
    static constexpr std::string_view APPLICATION_OCTED = "application/octet-stream"sv;

    static std::string_view FromFileExt(std::string_view ext) {
        if (ext_to_content.contains(ext)) {
            return ext_to_content.at(ext);
        }
        return APPLICATION_OCTED;
    }
private:
    static const std::unordered_map<std::string_view, std::string_view> ext_to_content;
};


struct RequestData {
    RequestData() = default;

    template <typename Body, typename Allocator>
    explicit RequestData(const http::request<Body, http::basic_fields<Allocator>>& req) {
        SetData(req);
    }

    template <typename Body, typename Allocator>
    void SetData(const http::request<Body, http::basic_fields<Allocator>>& req) {
        http_version = req.version();
        keep_alive = req.keep_alive();
        method = req.method();
        raw_uri = req.target();
        decoded_uri = util::DecodeURI(req.target());
        //decoded_uri = req.target();
        if (method == http::verb::post) {
            body = req.body();
            content_type = req[http::field::content_type];
        } else {
            body.reset();
            content_type.reset();
        }
        if (req.count(http::field::authorization)) {
            auth_token = ParseAuthToken(req);
        }        
    } 

    unsigned http_version{};
    bool keep_alive{};
    http::verb method{};
    std::string_view raw_uri;
    std::optional<std::string> decoded_uri;
    std::optional<std::string_view> body;
    std::optional<std::string_view> content_type;
    std::optional<service::Token> auth_token;    

private:
    template <typename Body, typename Allocator>
    std::optional<service::Token> ParseAuthToken(const http::request<Body, http::basic_fields<Allocator>>& req) {
        const std::string req_field = req[http::field::authorization];
        static const std::string auth_token_pattern = "[0-9a-fA-F]{32}"s;
        static const auto full_r = std::regex("^Bearer "s.append(auth_token_pattern).append("$"));
        if (!std::regex_match(req_field, full_r)) {
            return std::nullopt;
        }
        static const auto token_r = std::regex(auth_token_pattern);
        std::sregex_iterator begin{ req_field.cbegin(), req_field.cend(), token_r };
        std::sregex_iterator end{};
        if (std::distance(begin, end) != 1) {
            return std::nullopt;
        }
        return service::Token{std::move(begin->str())};
    }    
};

}
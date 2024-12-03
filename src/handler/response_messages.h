#pragma once

#include <string_view>

namespace http_request {


using namespace std::literals;

struct Codes {
    Codes() = delete;
    static constexpr std::string_view MapNotFound = "mapNotFound"sv;
    static constexpr std::string_view BadRequest = "badRequest"sv;
    static constexpr std::string_view FileNotFound = "fileNotFound"sv;
    static constexpr std::string_view InvalidMethod = "invalidMethod"sv;
    static constexpr std::string_view InvalidArgument = "invalidArgument"sv;
    static constexpr std::string_view InvalidAuth = "invalidToken"sv;
    static constexpr std::string_view InvalidToken = "unknownToken"sv;
    
};

struct Messages {
    Messages() = delete;
    static constexpr std::string_view MapNotFound       = "Map not found"sv;
    static constexpr std::string_view FileNotFound      = "File Not Found: "sv;
    static constexpr std::string_view BadRequest        = "Bad request"sv;
    static constexpr std::string_view BadContentType    = "Invalid content type"sv;
    static constexpr std::string_view InvalidURI        = "Invalid URI"sv;
    static constexpr std::string_view InvalidMethod     = "Invalid method"sv;
    static constexpr std::string_view JoinGameParse     = "Join game request parse error"sv;
    static constexpr std::string_view InvalidAuth       = "Authorization header is missing"sv;
    static constexpr std::string_view InvalidToken      = "Player token has not been found"sv;
    static constexpr std::string_view ActionParse       = "Failed to parse action"sv;
    static constexpr std::string_view TickParse         = "Failed to parse tick request JSON"sv;
    static constexpr std::string_view TickError         = "Failed error set tick"sv;
};

}
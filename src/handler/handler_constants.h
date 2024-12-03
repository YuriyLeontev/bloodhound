#pragma once

#include <string_view>
#include <filesystem>

namespace http_handler{

namespace fs = std::filesystem;
using namespace std::literals;

struct ApiTokens {
    ApiTokens() = delete;
    static constexpr std::string_view API       = "api"sv;
    static constexpr std::string_view V1        = "v1"sv;
    static constexpr std::string_view MAPS      = "maps"sv;
    static constexpr std::string_view GAME      = "game"sv;
    static constexpr std::string_view JOIN      = "join"sv;
    static constexpr std::string_view PLAYERS   = "players"sv;
    static constexpr std::string_view STATE     = "state"sv;
    static constexpr std::string_view PLAYER    = "player"sv;
    static constexpr std::string_view ACTION    = "action"sv;
    static constexpr std::string_view TICK      = "tick"sv;
    static constexpr std::string_view RECORDS   = "records"sv;
    static const fs::path api_root;
};

struct Constants {
    Constants() = delete;
    static constexpr std::string_view USER_NAME     = "userName"sv;
    static constexpr std::string_view MAP_ID        = "mapId"sv;    
    static constexpr std::string_view AUTH_TOKEN    = "authToken"sv;
    static constexpr std::string_view PLAYER_ID     = "playerId"sv;
    static constexpr std::string_view NAME          = "name"sv;
    static constexpr std::string_view POSITION      = "pos"sv;
    static constexpr std::string_view SPEED         = "speed"sv;
    static constexpr std::string_view DIRECTION     = "dir"sv;
    static constexpr std::string_view BAG           = "bag"sv;
    static constexpr std::string_view SCORE         = "score"sv;
    static constexpr std::string_view PLAYERS       = "players"sv;
    static constexpr std::string_view LOST_OBJECTS  = "lostObjects"sv;
    static constexpr std::string_view ID            = "id"sv;    
    static constexpr std::string_view TYPE          = "type"sv;    
    static constexpr std::string_view MOVE          = "move"sv;
    static constexpr std::string_view TIME_DELTA    = "timeDelta"sv;
    static constexpr std::string_view START         = "start"sv;
    static constexpr std::string_view MAX_ITEMS     = "maxItems"sv;
    static constexpr std::string_view PLAY_TIME     = "playTime"sv;    
};

}
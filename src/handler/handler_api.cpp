#include "handler_api.h"

#include "../loader/json_loader.h"

namespace http_handler{

using ErrorCode = http_request::ErrorBuilder::ErrorCode;

const fs::path ApiTokens::api_root = fs::path{"/"} / ApiTokens::API;

ApiHandler::ApiHandler(service::Service& service, const extra_data::ExtraData& extra_data) 
                                        : service_{service}
                                        , extra_data_{extra_data} {}

StringResponse ApiHandler::ResponseApiError(ErrorCode ec) const {
    return ErrorBuilder::MakeErrorResponse(ec, req_data_);
}

StringResponse ApiHandler::HandleMapsRequest(std::string_view version) const {
    if (version != ApiTokens::V1){
        return ErrorBuilder::MakeErrorResponse(ErrorCode::BadRequest, req_data_);
    }

    auto action = [this, version]() {
        return req_tokens_.empty()
            ? HandleAllMapsRequest(version)
            : HandleSingleMapRequest(version);
    };
    return ExecuteAllowedMethods(std::move(action), http::verb::get, http::verb::head);
}

StringResponse ApiHandler::HandleAllMapsRequest(std::string_view version) const {
    const auto& maps = service_.GetMaps();
    json::array json_maps;

    std::transform(make_move_iterator(maps.begin()), make_move_iterator(maps.end()), std::inserter(json_maps, json_maps.end()), 
                 [&](auto& map) {
                   return json_loader::MapAsJsonObject(map,extra_data_, true);
                 });

    auto body = json::serialize(json_maps);
    return MakeStringResponse(http::status::ok, body, req_data_, ContentType::APPLICATION_JSON);
}

StringResponse ApiHandler::HandleSingleMapRequest(std::string_view version) const {
    std::string_view map_id = req_tokens_.front(); req_tokens_.pop();
    if (!req_tokens_.empty()) {
        return ResponseApiError(ErrorCode::BadRequest);
    }
    model::Map::Id id{std::string{map_id}};
    const model::Map* map = service_.FindMap(id);
    if (!map) {
        return ResponseApiError(ErrorCode::MapNotFound);
    }
    auto body = json::serialize(json_loader::MapAsJsonObject(*map, extra_data_));
    return MakeStringResponse(http::status::ok, body, req_data_, ContentType::APPLICATION_JSON);
}


StringResponse ApiHandler::HandleGameRequest(std::string_view version) const {
    if (version != ApiTokens::V1){
        return ResponseApiError(ErrorCode::BadRequest);
    }
    if (req_tokens_.empty()) {
        return ResponseApiError(ErrorCode::BadRequest);
    }
    auto api_token = req_tokens_.front(); req_tokens_.pop();
    if (api_token == ApiTokens::JOIN && req_tokens_.empty()) {
        return HandlePlayerJoin(version);
    }
    if (api_token == ApiTokens::PLAYERS && req_tokens_.empty()) {
        return HandlePlayersRequest(version);
    }
    if (api_token == ApiTokens::STATE && req_tokens_.empty()) {
        return HandleStateRequest(version);
    }
    if (api_token == ApiTokens::PLAYER) {
        api_token = req_tokens_.front(); req_tokens_.pop();
        if (api_token == ApiTokens::ACTION && req_tokens_.empty()) {
            return HandlePlayerActionRequest(version);
        }
    }
    if (api_token == ApiTokens::TICK && req_tokens_.empty()) {
        return HandleTickRequest(version);
    }
    if (api_token.starts_with(ApiTokens::RECORDS)) {
        return HandleRecordsRequest(api_token, version);
    }    
    return ResponseApiError(ErrorCode::BadRequest);
}

StringResponse ApiHandler::HandlePlayerJoin(std::string_view version) const {
    auto action = [this]() {
        if (req_data_.content_type != ContentType::APPLICATION_JSON) {
            return ResponseApiError(ErrorCode::BadRequest);
        }
        std::error_code ec;
        json::value content = json::parse(req_data_.body.value(), ec);
        if (ec) {
            return ResponseApiError(ErrorCode::JoinGameParse);
        }
        if (!content.is_object() ||
            !content.as_object().contains(Constants::USER_NAME) ||
            !content.as_object().contains(Constants::MAP_ID) ||
            !content.as_object().at(Constants::USER_NAME).is_string() ||
            !content.as_object().at(Constants::MAP_ID).is_string() ||
            content.as_object().at(Constants::USER_NAME).as_string().empty()) {
            return ResponseApiError(ErrorCode::JoinGameParse);
        }
        std::string dog_name = content.as_object().at(Constants::USER_NAME).as_string().c_str();
        std::string map_id = content.as_object().at(Constants::MAP_ID).as_string().c_str();
        auto result = service_.JoinPlayer(model::Map::Id{map_id}, dog_name);
        if (!result.has_value()) {
            return ResponseApiError(ErrorCode::MapNotFound);
        }
        json::object player;
        player.emplace(Constants::AUTH_TOKEN, *result->first);
        player.emplace(Constants::PLAYER_ID, *result->second);
        auto body = json::serialize(player);
        return MakeStringResponse(http::status::ok, body, req_data_, ContentType::APPLICATION_JSON);
    };

    return ExecuteAllowedMethods(std::move(action), http::verb::post);

}

StringResponse ApiHandler::HandlePlayersRequest(std::string_view version) const {
    auto action = [this]() {
        return ExecuteAuthorized([this](const service::Token& token) {
            auto players = service_.GetPlayers(token);
            if (!players.has_value()) {
                return ResponseApiError(ErrorCode::PlayerTokenNotFound);
            }

            json::object json_players;
            for (const auto [id, name] : players.value()) {
                json::object player;
                player.emplace(Constants::NAME, name);
                json_players.emplace(std::to_string(*id), player);
            }
            auto body = json::serialize(json_players);
            return MakeStringResponse(http::status::ok, body, req_data_, ContentType::APPLICATION_JSON);
        });
    };
    return ExecuteAllowedMethods(std::move(action), http::verb::get, http::verb::head);
}

StringResponse ApiHandler::HandleStateRequest(std::string_view version) const{
    auto action = [this]() {
        return ExecuteAuthorized([this](const service::Token& token) {
            auto state = service_.GetGameState(token);
            if (!state.has_value()) {
                return ResponseApiError(ErrorCode::PlayerTokenNotFound);
            }

            static const std::unordered_map<model::Dog::Direction, std::string_view> direction_map{
                {model::Dog::Direction::NORTH, "U"sv},
                {model::Dog::Direction::SOUTH, "D"sv},
                {model::Dog::Direction::WEST,  "L"sv},
                {model::Dog::Direction::EAST,  "R"sv}
            };            

            json::object json_players_state;
            for (const auto& player : state->players) {
                json::object json_player;
                json_player.emplace(Constants::POSITION, json::array{player.pos.x, player.pos.y});
                json_player.emplace(Constants::SPEED, json::array{player.speed.x, player.speed.y});
                json_player.emplace(Constants::DIRECTION, direction_map.at(player.dir));
                json::array json_bag;
                for (auto loot_item : player.bag) {
                    json::object json_loot_item;
                    json_loot_item.emplace(Constants::ID, loot_item.id);
                    json_loot_item.emplace(Constants::TYPE, loot_item.type);
                    json_bag.emplace_back(std::move(json_loot_item));
                }
                json_player.emplace(Constants::BAG, std::move(json_bag));
                json_player.emplace(Constants::SCORE, std::move(player.scores));
                json_players_state.emplace(std::to_string(*player.id), std::move(json_player));
            }

            json::object json_loot_objects_state;
            for (const auto& loot_object : state->loot_objects) {
                json::object json_loot_object;
                json_loot_object.emplace(Constants::TYPE, loot_object.type);
                json_loot_object.emplace(Constants::POSITION, json::array{loot_object.pos.x, loot_object.pos.y});
                json_loot_objects_state.emplace(std::to_string(*loot_object.id), json_loot_object);
            }            

            json::object json_game_state;
            json_game_state.emplace(Constants::PLAYERS, std::move(json_players_state));
            json_game_state.emplace(Constants::LOST_OBJECTS, std::move(json_loot_objects_state));

            auto body = json::serialize(json_game_state);
            return MakeStringResponse(http::status::ok, body, req_data_, ContentType::APPLICATION_JSON);
        });
    };
    return ExecuteAllowedMethods(std::move(action), http::verb::get, http::verb::head);
}

StringResponse ApiHandler::HandlePlayerActionRequest(std::string_view version) const{
    const auto action = [this](const service::Token& token){
        if (req_data_.content_type != ContentType::APPLICATION_JSON) {
            return ResponseApiError(ErrorCode::BadContentType);
        }

        std::error_code ec;
        json::value content = json::parse(req_data_.body.value(), ec);
        if (ec) {
            return ResponseApiError(ErrorCode::ActionParse);
        }

        if (!content.is_object() ||
            !content.as_object().contains(Constants::MOVE)) {
            return ResponseApiError(ErrorCode::ActionParse);
        }
        static const std::unordered_map<std::string_view, model::Dog::Direction> direction_map{
            {"U"sv, model::Dog::Direction::NORTH},
            {"D"sv, model::Dog::Direction::SOUTH},
            {"L"sv, model::Dog::Direction::WEST},
            {"R"sv, model::Dog::Direction::EAST},
            {""sv,  model::Dog::Direction::STOP},
        };
        std::string dir = content.as_object().at(Constants::MOVE).as_string().c_str();
        if (!direction_map.contains(dir)) {
            return ResponseApiError(ErrorCode::JoinGameParse);
        }
        if (!service_.GameAction(token, direction_map.at(dir))) {
            return ResponseApiError(ErrorCode::PlayerTokenNotFound);
        }
        return MakeStringResponse(http::status::ok, {}, req_data_, ContentType::APPLICATION_JSON);
    };

    return ExecuteAllowedMethods([this, &action](){
        return ExecuteAuthorized([&action](const service::Token& token) {
            return action(token);
        });
    }, http::verb::post);
}

StringResponse ApiHandler::HandleTickRequest(std::string_view version) const{
    auto action = [this]() {
        if (req_data_.content_type != ContentType::APPLICATION_JSON) {
            return ResponseApiError(ErrorCode::BadRequest);
        }
        std::error_code ec;
        json::value content = json::parse(req_data_.body.value(), ec);
        if (ec) {
            return ResponseApiError(ErrorCode::TickParse);
        }
        if (!content.is_object() ||
            !content.as_object().contains(Constants::TIME_DELTA) ||
            !content.as_object().at(Constants::TIME_DELTA).is_int64()) {
            return ResponseApiError(ErrorCode::TickParse);
        }
        auto timeDelta = content.as_object().at(Constants::TIME_DELTA).as_int64();
        if (timeDelta < 0) {
            return ResponseApiError(ErrorCode::TickParse);
        }        

        auto tick = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<size_t, std::milli>(timeDelta));
        if (service_.TimeTick(tick)) {
            return MakeStringResponse(http::status::ok, {}, req_data_, ContentType::APPLICATION_JSON);
        }
        return ResponseApiError(ErrorCode::TickError);
    };

    return ExecuteAllowedMethods(std::move(action), http::verb::post);
}

int ExtractParameterValue(std::string_view api_token, std::string_view parameter) {
    size_t start = api_token.find(parameter);
    if (start == std::string::npos) {
        return 0;
    }
    start += parameter.size();
    if (start == api_token.size() || api_token[start] != '=') {
        throw std::runtime_error("Value not found");
    }
    size_t end = api_token.find_first_of('&', start);
    if (end == std::string::npos) {
        end = api_token.size();
    }
    return std::stoi(std::string{api_token.substr(start + 1, end - start)});
}

StringResponse ApiHandler::HandleRecordsRequest(std::string_view api_token, std::string_view version) const {
    const auto action = [this, api_token](){
        int start, max_items;
        try {
            start     = ExtractParameterValue(api_token, Constants::START);
            max_items = ExtractParameterValue(api_token, Constants::MAX_ITEMS);
        } catch (...) {
            return ResponseApiError(ErrorCode::BadRequest);
        }
        if (max_items > 100) {
            return ResponseApiError(ErrorCode::BadRequest);
        }
        if (max_items == 0) {
            max_items = 100;
        }
        auto players = service_.Records(start, max_items);
        json::array json_players;
        for (const auto& player : players) {
            json::object json_player;
            json_player.emplace(Constants::NAME, player.GetName());
            json_player.emplace(Constants::SCORE, player.GetScore());
            json_player.emplace(Constants::PLAY_TIME, player.PlayTime()*1./1000);
            json_players.push_back(std::move(json_player));
        }
        return MakeStringResponse(http::status::ok, json::serialize(json_players), req_data_, ContentType::APPLICATION_JSON);
    };

    return ExecuteAllowedMethods([this, &action](){
        return action();
    }, http::verb::get, http::verb::head);
}

}
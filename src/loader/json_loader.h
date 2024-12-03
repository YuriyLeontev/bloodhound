#pragma once

#include <boost/json.hpp>
#include <boost/describe.hpp>

#include <filesystem>
#include <map>

#include "../model/model.h"
#include "extra_data.h"

namespace json_loader {

using namespace std::literals;

struct Fields {
    Fields() = delete;
    static constexpr std::string_view maps = "maps"sv;
    static constexpr std::string_view defaultDogSpeed = "defaultDogSpeed"sv;
    static constexpr std::string_view lootGeneratorConfig = "lootGeneratorConfig"sv;
    static constexpr std::string_view lootTypes = "lootTypes"sv;
    static constexpr std::string_view defaultBagCapacity = "defaultBagCapacity"sv;
    static constexpr std::string_view dogRetirementTime = "dogRetirementTime"sv;
};

struct MapFields {
    MapFields() = delete;
    static constexpr std::string_view id          = "id"sv;
    static constexpr std::string_view name        = "name"sv;
    static constexpr std::string_view roads       = "roads"sv;
    static constexpr std::string_view buildings   = "buildings"sv;
    static constexpr std::string_view offices     = "offices"sv;
    static constexpr std::string_view dogSpeed    = "dogSpeed"sv;
    static constexpr std::string_view bagCapacity = "bagCapacity"sv;
};

struct RoadFields {
    RoadFields() = delete;
    static constexpr std::string_view x0 = "x0"sv;
    static constexpr std::string_view y0 = "y0"sv;
    static constexpr std::string_view x1 = "x1"sv;
    static constexpr std::string_view y1 = "y1"sv;
};

struct BuildingFields {
    BuildingFields() = delete;
    static constexpr std::string_view x = "x"sv;
    static constexpr std::string_view y = "y"sv;
    static constexpr std::string_view w = "w"sv;
    static constexpr std::string_view h = "h"sv;
};

struct OfficeFields {
    OfficeFields() = delete;
    static constexpr std::string_view id        = "id"sv;
    static constexpr std::string_view x         = "x"sv;
    static constexpr std::string_view y         = "y"sv;
    static constexpr std::string_view offsetX   = "offsetX"sv;
    static constexpr std::string_view offsetY   = "offsetY"sv;
};

struct LootGeneratorFields {
    LootGeneratorFields() = delete;
    static constexpr std::string_view period       = "period"sv;
    static constexpr std::string_view probability = "probability"sv;
};

struct LootTypesFields {
    LootTypesFields() = delete;
    static constexpr std::string_view value       = "value"sv;
};



boost::json::value SetJsonDataError(std::string_view code, std::string_view message);

boost::json::object MapAsJsonObject(const model::Map& map, const extra_data::ExtraData& extra_data, bool short_info = false);


std::pair<model::Game, extra_data::ExtraData> LoadGame(const std::filesystem::path& json_path);


}  // namespace json_loader
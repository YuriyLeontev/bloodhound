#include "json_loader.h"

#include <chrono>
#include <string>
#include <fstream>


namespace json_loader {

namespace fs = std::filesystem;
namespace json = boost::json;
using namespace std::literals;

static std::string ReadFile(const std::ifstream& file) {
    if (!file.is_open()) {
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static std::string GetObjectFieldAsString(const json::value& obj, std::string_view field) {
    return obj.at(field).as_string().c_str();
}

static geom::Dimension GetObjectFieldAsDimension(const json::value& obj, std::string_view field) {
    return static_cast<geom::Dimension>(obj.at(field).as_int64());
}

static void AddRoad(const json::value& json_road, model::Map& map) {
    bool has_x1 = json_road.as_object().contains(RoadFields::x1);
    bool has_y1 = json_road.as_object().contains(RoadFields::y1);
    if ((has_x1 && has_y1) || (!has_x1 && !has_y1)) {
        std::ostringstream os;
        os << "Map ["sv << *map.GetId() <<"] \'"sv << map.GetName() << "\'. Invalid road"sv;
        throw std::invalid_argument("");
    }
    geom::Point start{.x = GetObjectFieldAsDimension(json_road, RoadFields::x0),
                       .y = GetObjectFieldAsDimension(json_road, RoadFields::y0)};
    if (has_x1) {
        map.AddRoad({ model::Road::HORIZONTAL, start, GetObjectFieldAsDimension(json_road, RoadFields::x1)});
    }
    if (has_y1) {
        map.AddRoad({ model::Road::VERTICAL, start, GetObjectFieldAsDimension(json_road, RoadFields::y1)});
    }
}

static void AddBuilding(const json::value& json_building, model::Map& map) {
    geom::Point position{.x = GetObjectFieldAsDimension(json_building, BuildingFields::x),
                          .y = GetObjectFieldAsDimension(json_building, BuildingFields::y)};
    model::Size size{.width = GetObjectFieldAsDimension(json_building, BuildingFields::w),
                     .height = GetObjectFieldAsDimension(json_building, BuildingFields::h)};
    map.AddBuilding(model::Building{ model::Rectangle{ .position = position,
                                                       .size = size } });
}

static void AddOffice(const json::value& json_office, model::Map& map) {
    std::string id = GetObjectFieldAsString(json_office, OfficeFields::id);
    geom::Point position{.x = GetObjectFieldAsDimension(json_office, OfficeFields::x),
                          .y = GetObjectFieldAsDimension(json_office, OfficeFields::y)};
    model::Offset offset{.dx = GetObjectFieldAsDimension(json_office, OfficeFields::offsetX),
                         .dy = GetObjectFieldAsDimension(json_office, OfficeFields::offsetY)};
    map.AddOffice(model::Office{model::Office::Id{std::move(id)}, position, offset});
}

json::value LoadJsonData(const std::filesystem::path& json_path) {
    // Загружаем модель игры из файла
    std::error_code ec;
    if (!fs::exists(json_path, ec) || ec) {
        std::ostringstream message;
        message << "File \"" << json_path.string() << "\" not found: "sv << ec.message();
        throw std::filesystem::filesystem_error(message.str(), ec);
    }
    // Загружаем содержимое файла json_path, в виде строки
    std::ifstream config_file(json_path);
    std::string json_string = ReadFile(config_file);
    // Парсим строку как JSON
    return json::parse(json_string);
}


/// @brief Загрузка конфигурационных данных игры
/// @param json_path Путь к файлу json в файловой системе
/// @return Game object
std::pair<model::Game, extra_data::ExtraData> LoadGame(const std::filesystem::path& json_path) {
    model::Game game;
    extra_data::ExtraData data;

    auto input_json = LoadJsonData(json_path);

    const double default_dog_speed = input_json.as_object().contains(Fields::defaultDogSpeed)
        ? input_json.at(Fields::defaultDogSpeed).as_double()
        : 1.;

    const double dog_retirement_time = input_json.as_object().contains(Fields::dogRetirementTime)
        ? input_json.at(Fields::dogRetirementTime).as_double()
        : 60.;

    const int default_bag_capacity = input_json.as_object().contains(Fields::defaultBagCapacity)
        ? input_json.at(Fields::defaultBagCapacity).as_int64()
        : 3;        

    game.SetDogRetirementTime(static_cast<size_t>(dog_retirement_time * 1000));

    game.SetLootGeneratorParams(
        input_json.at(Fields::lootGeneratorConfig).at(LootGeneratorFields::period).as_double(),
        input_json.at(Fields::lootGeneratorConfig).at(LootGeneratorFields::probability).as_double()
    );

    for (const auto& json_map : input_json.at(Fields::maps).as_array()) {

        model::Map::Id id(GetObjectFieldAsString(json_map, MapFields::id));
        std::string name = GetObjectFieldAsString(json_map, MapFields::name);

        const double dog_speed = json_map.as_object().contains(MapFields::dogSpeed)
            ? json_map.at(MapFields::dogSpeed).as_double()
            : default_dog_speed;

        const int bag_capacity = json_map.as_object().contains(MapFields::bagCapacity)
            ? json_map.at(MapFields::bagCapacity).as_double()
            : default_bag_capacity;

        model::Map map(std::move(id), std::move(name));
        map.SetDogSpeed(dog_speed).SetDogBagCapacity(bag_capacity);

        auto loot_types = json_map.at(Fields::lootTypes).as_array();
        for (const auto& json_loot : loot_types) {
            map.AddLootWorth(json_loot.at(LootTypesFields::value).as_int64());
        }

        

        data.map_id_to_loot_types[map.GetId()] = std::move(loot_types);
        for (const auto& json_road : json_map.at(MapFields::roads).as_array()) {
            AddRoad(json_road, map);
        }
        for (const auto& json_building : json_map.at(MapFields::buildings).as_array()) {
            AddBuilding(json_building, map);
        }
        for (const auto& json_office : json_map.at(MapFields::offices).as_array()) {
            AddOffice(json_office, map);
        }
        game.AddMap(std::move(map));
    }

    return {std::move(game), std::move(data)};
}


static void JsonifyMainMapInfo(const model::Map& map, json::object& json_map) {
    json_map.emplace(json_loader::MapFields::id, *map.GetId());
    json_map.emplace(json_loader::MapFields::name, map.GetName());
}

static void JsonifyRoads(const model::Map& map, json::object& json_map) {
    json::array json_roads;
    for (const auto& road : map.GetRoads()) {
        json::object json_road;
        geom::Point start = road.GetStart();
        json_road.emplace(json_loader::RoadFields::x0, start.x);
        json_road.emplace(json_loader::RoadFields::y0, start.y);
        geom::Point end = road.GetEnd();
        if (road.IsHorizontal()) {
            json_road.emplace(json_loader::RoadFields::x1, end.x);
        } else {
            json_road.emplace(json_loader::RoadFields::y1, end.y);
        }
        json_roads.push_back(std::move(json_road));
    }
    json_map.emplace(json_loader::MapFields::roads, json_roads);
}

static void JsonifyBuildings(const model::Map& map, json::object& json_map) {
    json::array json_offices;
    for (const auto& office : map.GetOffices()) {
        json::object json_office;
        json_office.emplace(json_loader::OfficeFields::id, *office.GetId());
        geom::Point position = office.GetPosition();
        json_office.emplace(json_loader::OfficeFields::x, position.x);
        json_office.emplace(json_loader::OfficeFields::y, position.y);
        model::Offset offset = office.GetOffset();
        json_office.emplace(json_loader::OfficeFields::offsetX, offset.dx);
        json_office.emplace(json_loader::OfficeFields::offsetY, offset.dy);
        json_offices.push_back(std::move(json_office));
    }
    json_map.emplace(json_loader::MapFields::offices, json_offices);
}

static void JsonifyOffices(const model::Map& map, json::object& json_map) {
    json::array json_buildings;
    for (const auto& building : map.GetBuildings()) {
        json::object json_building;
        model::Rectangle rect = building.GetBounds();
        json_building.emplace(json_loader::BuildingFields::x, rect.position.x);
        json_building.emplace(json_loader::BuildingFields::y, rect.position.y);
        json_building.emplace(json_loader::BuildingFields::w, rect.size.width);
        json_building.emplace(json_loader::BuildingFields::h, rect.size.height);
        json_buildings.push_back(std::move(json_building));
    }
    json_map.emplace(json_loader::MapFields::buildings, json_buildings);
}

json::object MapAsJsonObject(const model::Map& map, const extra_data::ExtraData& extra_data, bool short_info /* = false */) {
    json::object json_map;
    JsonifyMainMapInfo(map, json_map);
    if (short_info) {
        return json_map;
    }
    json_map.emplace(json_loader::Fields::lootTypes, extra_data.map_id_to_loot_types.at(map.GetId()));
    JsonifyRoads(map, json_map);
    JsonifyOffices(map, json_map);
    JsonifyBuildings(map, json_map);
    return json_map;
}

}  // namespace json_loader

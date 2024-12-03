#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <list>
#include <memory>
#include <chrono>
#include <optional>

#include "geom.h"
#include "loot_generator.h"
#include "collision_detector.h"
#include "../util/tagged.h"

#include <boost/signals2.hpp>

namespace model {

static constexpr double DOG_WIDTH = 0.6;
static constexpr double OFFICE_WIDTH = 0.5;
static constexpr double LOOT_WIDTH = 0.;

struct Size {
    geom::Dimension width, height;
};

struct Rectangle {
    geom::Point position;
    Size size;
};

struct Offset {
    geom::Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, geom::Point start, geom::Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y}
        , x_range_{std::min(start.x, end_x), std::max(start.x, end_x)}
        , y_range_{start.y, start.y} {
        CalculateDimentions();
    }

    Road(VerticalTag, geom::Point start, geom::Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y}
        , x_range_{start.x, start.x}
        , y_range_{std::min(start.y, end_y), std::max(start.y, end_y)} {
        CalculateDimentions();
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    geom::Point GetStart() const noexcept {
        return start_;
    }

    geom::Point GetEnd() const noexcept {
        return end_;
    }

    using Range = std::pair<geom::Dimension, geom::Dimension>;
    const Range& GetRangeX() const noexcept {
        return x_range_;
    }

    const Range& GetRangeY() const noexcept {
        return y_range_;
    }    

    struct Rect{geom::Vec2D p1, p2;};
    const Rect& GetAbsDimentions() const noexcept {
        return abs_dimentions_;
    }

private:
    void CalculateDimentions() {
        abs_dimentions_.p1.x = x_range_.first - ROAD_SIDE;
        abs_dimentions_.p1.y = y_range_.first - ROAD_SIDE;
        abs_dimentions_.p2.x = x_range_.second + ROAD_SIDE;
        abs_dimentions_.p2.y = y_range_.second + ROAD_SIDE;
    }


private:
    static constexpr double ROAD_SIDE = 0.4;

    geom::Point start_;
    geom::Point end_;

    Range x_range_;
    Range y_range_;
    Rect abs_dimentions_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    static constexpr double COLLISION_RADIUS = OFFICE_WIDTH / 2;
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, geom::Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    geom::Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    geom::Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void   AddLootWorth(size_t lootWorth);
    size_t GetLootWorth(size_t lootType) const noexcept;
    size_t CountLootWorth() const noexcept;

    Map&  SetDogSpeed(double value);
    double GetDogSpeed() const noexcept;

    Map&  SetDogBagCapacity(size_t value);
    size_t GetDogBagCapacity() const noexcept;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    double dogSpeed_;
    size_t bagCapacity_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    std::vector<size_t> lootWorth_; 
};

// LootObject
class LootObject {
public:
    static constexpr double COLLISION_RADIUS = LOOT_WIDTH / 2;

    using Id = util::Tagged<size_t, LootObject>;

    LootObject() = default;
    LootObject(Id id, size_t type, size_t worth) noexcept;

    const Id& GetId() const noexcept;
    size_t GetType() const noexcept;
    size_t GetWorth() const noexcept;

private:
    Id id_{0};
    size_t type_;
    size_t worth_;
};


class Dog {
public:
    enum class Direction {
        NORTH,
        SOUTH,
        WEST,
        EAST,
        STOP
    };
    static constexpr double COLLISION_RADIUS = DOG_WIDTH / 2;


    using Id = util::Tagged<size_t, Dog>;
    Dog() = default;
    Dog(const Dog&) = default;
    Dog(Dog&&) = default;

    Dog(Dog::Id id, std::string name, geom::Vec2D coords, Direction direction = Direction::NORTH, geom::Vec2D speed = {} ) noexcept;

    Dog& operator=(const Dog&) = delete;
    Dog& operator=(Dog&&) = default;

    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const geom::Vec2D& GetCoordinates() const noexcept;
    const geom::Vec2D& GetPrevCoordinates() const noexcept;
    const geom::Vec2D& GetSpeed() const noexcept;
    const Direction GetDirection() const noexcept;

    void SetSpeed(double speed);
    void SetDirection(Direction dir);
    void SetCoordinates(geom::Vec2D coord);

    void Stop();

    bool IsStoped() const noexcept;
    void AddTick(size_t tick);

    size_t GetHoldingPeriod() const noexcept;
    size_t GetTimeInGame() const noexcept;    

    using Bag = std::vector<LootObject>;
    const Bag& GetBag() const;
    size_t LootCountInBag() const noexcept;
    void   AddLoot(LootObject loot);
    void   DropBag();

    size_t GetScores() const noexcept;
    void   SetScores(size_t score);

private:
    Id id_{0u};
    std::string name_;    
    Direction direction_;
    geom::Vec2D coords_;
    geom::Vec2D speed_;
    geom::Vec2D prev_coords_;

    Bag bag_;

    size_t scores_ = 0;
    size_t holding_time_ = 0;
    size_t time_in_game_ = 0;    
};

using DogRetire = boost::signals2::signal<void(model::Dog::Id dog, const model::Map::Id& map)>;

class GameSession {
public:
    using Id = util::Tagged<size_t, GameSession>;
    using DogPtr = std::shared_ptr<Dog>;

    GameSession(const Map* map, size_t index, bool random_spawn, const loot_gen::LootGeneratorParams& loot_gen_params,
        const size_t dog_retirement_time, DogRetire& do_on_retire_,
        size_t dog_start_id = 0, size_t loot_object_start_id = 0) noexcept;

    const Id& GetId() const noexcept;

    DogPtr NewDog(std::string name);
    DogPtr AddDog(Dog dog);
    DogPtr GetDogById(Dog::Id id) const;    
    const std::list<DogPtr>& GetDogs() const;

    void AddLoot(LootObject obj, geom::Vec2D coords);

    const Map& GetMap() const {return *map_;}

    void OnTick(std::chrono::milliseconds tick);

    geom::Vec2D GetLootCoordsById(LootObject::Id id) const;

    using LootObjectIdToObject = std::unordered_map<LootObject::Id, LootObject, util::TaggedHasher<LootObject::Id>>;
    const LootObjectIdToObject& GetLootObjects() const;

    struct DynamicStateContent {
        using LootObjects = std::vector<std::pair<LootObject, geom::Vec2D>>;

        Map::Id map_id{""};
        GameSession::Id session_id{0u};
        std::list<Dog> dogs;
        LootObjects loot_objects;
        size_t dogs_join;
        size_t objects_spawned;
    };

    DynamicStateContent GetDynamicStateContent() const;

private:
    geom::Vec2D GetRandomPointOnRandomRoad() const;
    geom::Vec2D GetDogSpawnPoint();

    void Move(Dog& dog, std::chrono::milliseconds time_delta) const;

    void SpawnLoot(std::chrono::milliseconds tick);
    void SpawnLootObject();

    void HandleCollisions();

    void HandleLootCollection(DogPtr dog, LootObject::Id);
    void HandleLootDrop(DogPtr dog);

    std::optional<LootObject> ExtractLootObject(LootObject::Id id);

    void RetireDogs();

private:
    const Map* map_;
    Id id_;

    size_t dogs_join_;
    size_t road_count_;
    size_t objects_spawned_;

    bool random_spawn_;
    loot_gen::LootGenerator loot_generator_;
    
    size_t dog_retirement_time_;
    DogRetire& do_on_retire_;
    std::vector<Dog::Id> dogs_to_retire_;
    
    using DogIdToDog = std::unordered_map<Dog::Id, std::list<DogPtr>::iterator, util::TaggedHasher<Dog::Id>>;
    DogIdToDog dog_id_to_dog_;

    std::list<DogPtr> dogs_;

    using CoordsToRoad = std::unordered_map<geom::Dimension, std::unordered_multimap<geom::Dimension, const model::Road*>>;
    CoordsToRoad coords_to_roads_;

    LootObjectIdToObject loot_obj_id_to_obj_;

    using LootObjectIdToCoords = std::unordered_map<LootObject::Id, geom::Vec2D, util::TaggedHasher<LootObject::Id>>;
    LootObjectIdToCoords loot_obj_id_to_coords_;
};

class Game {
public:
    using Maps = std::vector<Map>;
    void AddMap(Map map);
    const Maps& GetMaps() const noexcept;
    const Map* FindMap(const Map::Id& id) const noexcept;
    GameSession* GetGameSessionByMapId(const Map::Id& id);
    GameSession* AddGameSession(const Map::Id& id, size_t index,
            size_t dog_start_id = 0, size_t loot_object_start_id = 0);

    void OnTick(std::chrono::milliseconds time_delta);

    void SetRandomSpawn(bool isRandom);
    void SetLootGeneratorParams(double period, double probability);
    void SetDogRetirementTime(size_t dog_retirement_time);

    using GameState = std::vector<GameSession::DynamicStateContent>;
    GameState GetGameState() const;

    [[nodiscard]] boost::signals2::connection RetireListener(const DogRetire::slot_type& handler) {
        return do_on_retire_.connect(handler);
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapIdToSession = std::unordered_map<Map::Id, GameSession, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    MapIdToSession map_id_to_session_;

    size_t last_session_index_ = 0;

    bool random_spawn_ = false;
    loot_gen::LootGeneratorParams loot_generator_params_;
    size_t dog_retirement_time_;
    DogRetire do_on_retire_;
};

}  // namespace model

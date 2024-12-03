#include "model.h"

#include <stdexcept>
#include <random>

namespace model {
using namespace std::literals;

/* Map */

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Map::AddLootWorth(size_t lootWorth){
    lootWorth_.push_back(lootWorth);
}

size_t Map::GetLootWorth(size_t lootType) const noexcept{
    return lootWorth_.at(lootType);
}

size_t Map::CountLootWorth() const noexcept{
    return lootWorth_.size();
}

Map& Map::SetDogSpeed(double value){
    dogSpeed_ = value;
    return *this;
}

double Map::GetDogSpeed() const noexcept{
    return dogSpeed_;
}

Map& Map::SetDogBagCapacity(size_t value){
    bagCapacity_ = value;
    return *this;
}

size_t Map::GetDogBagCapacity() const noexcept {
    return bagCapacity_;
}

/* Game */

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

void Game::OnTick(std::chrono::milliseconds time_delta){
    // Отправляем время по всем сессиям
    for (auto& [map, session] : map_id_to_session_) {
        session.OnTick(time_delta);
    }
}


/* Dog */

Dog::Dog(Dog::Id id, std::string name, geom::Vec2D coords, Direction direction, geom::Vec2D speed) noexcept
    : id_(id)
    , name_(std::move(name))
    , direction_{direction}
    , coords_{coords}
    , prev_coords_{coords}
    , speed_(speed)
    , scores_{0} { }


const Dog::Id& Dog::GetId() const noexcept {
    return id_;
}

const std::string& Dog::GetName() const noexcept {
    return name_;
}

const geom::Vec2D& Dog::GetCoordinates() const noexcept {
    return coords_;
}

const geom::Vec2D& Dog::GetPrevCoordinates() const noexcept {
    return prev_coords_;
}

const geom::Vec2D& Dog::GetSpeed() const noexcept{
    return speed_;
}

const Dog::Direction Dog::GetDirection() const noexcept{
    return direction_;
}

void Dog::SetSpeed(double speed) {
    speed_.x = (direction_ == Direction::NORTH || direction_ == Direction::SOUTH )
                ? 0.
                : (direction_ == Direction::EAST ? speed : -speed);
    speed_.y = (direction_ == Direction::WEST || direction_ == Direction::EAST )
                ? 0.
                : (direction_ == Direction::SOUTH ? speed : -speed);
}


void Dog::SetDirection(Direction dir){
    direction_ = dir;
}

void Dog::SetCoordinates(geom::Vec2D coord){
    prev_coords_ = coords_;
    coords_ = coord;
}

void Dog::Stop() {
    speed_.x = speed_.y = 0.;
}

bool Dog::IsStoped() const noexcept {
    return speed_.x == 0. && speed_.y == 0.;
}

void Dog::AddTick(size_t tick) {
    time_in_game_ += tick;
    if (IsStoped()) {
        holding_time_ += tick;
    }
}

size_t Dog::GetHoldingPeriod() const noexcept {
    return holding_time_;
}

size_t Dog::GetTimeInGame() const noexcept {
    return time_in_game_;
}

const Dog::Bag& Dog::GetBag() const{
    return bag_;
}

size_t Dog::LootCountInBag() const noexcept{
    return bag_.size();
}

void Dog::AddLoot(LootObject loot){
    bag_.emplace_back(std::move(loot));
}

void Dog::DropBag(){
    scores_ += std::accumulate(bag_.begin(),bag_.end(),0,
        [this](size_t lhs, const LootObject& rhs) {
            return lhs + rhs.GetWorth();
        });

    bag_.clear();
}

size_t Dog::GetScores() const noexcept{
    return scores_;
}

void Dog::SetScores(size_t scores){
    scores_ += scores;
}

/* LootObject */

LootObject::LootObject(Id id, size_t type, size_t worth) noexcept
    : id_{id}
    , type_{type}
    , worth_{worth} {
}

const LootObject::Id& LootObject::GetId() const noexcept {
    return id_;
}

size_t LootObject::GetType() const noexcept {
    return type_;
}

size_t LootObject::GetWorth() const noexcept {
    return worth_;
}


/* Game */

GameSession* Game::GetGameSessionByMapId(const Map::Id& id) {
    if (auto session = map_id_to_session_.find(id); session != map_id_to_session_.end()) {
        return &session->second;
    }
    if (auto map = map_id_to_index_.find(id); map != map_id_to_index_.end()) {
        GameSession session(
            &maps_[map->second],
            last_session_index_++,
            random_spawn_,
            loot_generator_params_,
            dog_retirement_time_,
            do_on_retire_
        );
        return &(map_id_to_session_.emplace(id, std::move(session)).first->second);
    }
    return nullptr;
}

GameSession* Game::AddGameSession(const Map::Id& id, size_t index,
        size_t dog_start_id, size_t loot_object_start_id) {
    if (auto map = map_id_to_index_.find(id); map != map_id_to_index_.end()) {
        GameSession session(
            &maps_[map->second],
            index,
            random_spawn_,
            loot_generator_params_,
            dog_retirement_time_,
            do_on_retire_,
            dog_start_id,
            loot_object_start_id
        );
        return &(map_id_to_session_.emplace(id, std::move(session)).first->second);
    } else {
        throw std::runtime_error("Map not found");
    }
    return nullptr;
}

void Game::SetRandomSpawn(bool isRandom){
    random_spawn_ = isRandom;
}

void Game::SetLootGeneratorParams(double period, double probability){
    loot_generator_params_.period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(period));
    loot_generator_params_.probability = probability;
}

void Game::SetDogRetirementTime(size_t dog_retirement_time) {
    dog_retirement_time_ = dog_retirement_time;
}

Game::GameState Game::GetGameState() const {
    GameState state;
    state.reserve(map_id_to_session_.size());
    for (const auto& [id, session] : map_id_to_session_) {
        state.emplace_back(session.GetDynamicStateContent());
    }
    return state;
}


/* GameSession */

GameSession::GameSession(const Map* map, size_t index, 
            bool random_spawn, 
            const loot_gen::LootGeneratorParams& loot_gen_params,
            const size_t dog_retirement_time,
            DogRetire &do_on_retire_,
            size_t dog_start_id, 
            size_t loot_object_start_id ) noexcept
            : map_(map)
            , id_(index)
            , random_spawn_(random_spawn)
            , loot_generator_{loot_gen_params.period, loot_gen_params.probability}
            , dog_retirement_time_{dog_retirement_time}
            , do_on_retire_{do_on_retire_}
            , objects_spawned_{loot_object_start_id}
            , dogs_join_{dog_start_id}
            , road_count_(map->GetRoads().size()) {

    for (const auto& road : map_->GetRoads()) {
        for (int x = road.GetRangeX().first; x <= road.GetRangeX().second; ++x) {
            for (int y = road.GetRangeY().first; y <= road.GetRangeY().second; ++y) {
                coords_to_roads_[x].emplace(y, &road);
            }
        }
    }
}

const GameSession::Id& GameSession::GetId() const noexcept {
    return id_;
}

const GameSession::LootObjectIdToObject& GameSession::GetLootObjects() const {
    return loot_obj_id_to_obj_;
}

geom::Vec2D GameSession::GetLootCoordsById(LootObject::Id id) const {
    return loot_obj_id_to_coords_.at(id);
}


GameSession::DogPtr GameSession::NewDog(std::string name){
    size_t index = dogs_join_++;
    return AddDog({Dog::Id{index}, std::move(name),GetDogSpawnPoint()});    
}

GameSession::DogPtr GameSession::AddDog(Dog dog) {
    auto dog_it = dogs_.emplace(dogs_.end(), std::make_unique<Dog>(std::move(dog)));
    auto [it, inserted] = dog_id_to_dog_.emplace((*dog_it)->GetId(), dog_it);
    if (!inserted) {
        throw std::runtime_error("Dog already exists");
    }
    return *dog_it;
}

GameSession::DogPtr GameSession::GetDogById(Dog::Id id) const {
    if (auto it = dog_id_to_dog_.find(id); it != dog_id_to_dog_.end()) {
        return *(it->second);
    }
    return nullptr;
}

const std::list<GameSession::DogPtr>& GameSession::GetDogs() const {
    return dogs_;
}

void GameSession::AddLoot(LootObject obj, geom::Vec2D coords) {
    if (loot_obj_id_to_obj_.contains(obj.GetId())) {
        throw std::runtime_error("Loot object already exists");
    }
    loot_obj_id_to_coords_.emplace(obj.GetId(), coords);
    loot_obj_id_to_obj_.emplace(obj.GetId(), obj);
}

geom::Vec2D GameSession::GetRandomPointOnRandomRoad() const{
    static std::random_device random_device_;
    // Выбираем рандомную дорогу
    std::uniform_int_distribution<size_t> road_d(0, road_count_ - 1);
    size_t road_index = road_d(random_device_);
    if(road_index >= road_count_) {
        road_index = 0;
    }
    const auto& road = map_->GetRoads().at(road_index);
    // Выбираем рандомные координаты на дороге
    double x = std::uniform_real_distribution<double>{road.GetAbsDimentions().p1.x, road.GetAbsDimentions().p2.x}(random_device_);
    double y = std::uniform_real_distribution<double>{road.GetAbsDimentions().p1.y, road.GetAbsDimentions().p2.y}(random_device_);
    return geom::Vec2D{x,y};
}

geom::Vec2D GameSession::GetDogSpawnPoint(){
    if (random_spawn_) {
        return GetRandomPointOnRandomRoad();
    } else {
        const auto& road = map_->GetRoads().front();
        double x = road.GetStart().x;
        double y = road.GetStart().y;
        return {x,y};
    }
}

void GameSession::SpawnLootObject() {
    size_t index = objects_spawned_++;
    static std::random_device rd;
    size_t type = std::uniform_int_distribution<size_t>{0, map_->CountLootWorth() - 1}(rd);
    auto [it, inserted] = loot_obj_id_to_obj_.emplace(
        LootObject::Id{index},
        LootObject(LootObject::Id{index}, type, map_->GetLootWorth(type))
    );
    if (!inserted) {
        throw std::runtime_error("Loot object already exists");
    }
    loot_obj_id_to_coords_[it->first] = GetRandomPointOnRandomRoad();
}

void GameSession::SpawnLoot(std::chrono::milliseconds tick) {
    int objects_count = loot_generator_.Generate(
        tick,
        loot_obj_id_to_obj_.size(),
        dogs_.size()
    );
    while (objects_count--) {
        SpawnLootObject();
    }
}

void GameSession::OnTick(std::chrono::milliseconds tick){
    for (auto dog : dogs_) {
        Move(*dog, tick);
        if (dog->IsStoped() && dog->GetHoldingPeriod() >= dog_retirement_time_) {
            dogs_to_retire_.push_back(dog->GetId());
        }        
    }
    RetireDogs();
    HandleCollisions();
    SpawnLoot(tick);    
}

void GameSession::RetireDogs() {
    for (Dog::Id dog_id : dogs_to_retire_) {
        do_on_retire_(dog_id, map_->GetId());
        auto nh = dog_id_to_dog_.extract(dog_id);
        dogs_.erase(nh.mapped());
    }
    dogs_to_retire_.clear();
}

static geom::Dimension RoundRoadCoord(double coord) {
    static constexpr double mid = 0.5;
    return (coord - std::floor(coord) < mid) ? std::floor(coord) : std::ceil(coord);
}

static geom::Point RoundRoadCoords(const geom::Vec2D& coords) {
    return {RoundRoadCoord(coords.x),RoundRoadCoord(coords.y)};
}

static double PossibleMoveDist(const geom::Vec2D& from, const Road& road, Dog::Direction dir) {
    switch (dir) {
    case Dog::Direction::NORTH:
        return road.GetAbsDimentions().p1.y - from.y;
    case Dog::Direction::SOUTH:
        return road.GetAbsDimentions().p2.y - from.y;
    case Dog::Direction::WEST:
        return road.GetAbsDimentions().p1.x - from.x;
    case Dog::Direction::EAST:
        return road.GetAbsDimentions().p2.x - from.x;
    default:
        return {};
    }
}

void GameSession::HandleCollisions() {
    using namespace collision_detector;

    std::vector<collision_detector::Gatherer> gatherers;
    std::vector<collision_detector::Item> items;

    // add gatherer
    std::unordered_map<size_t, DogPtr> gatherer_id_to_dog;
    for (const auto & dog : dogs_) {
        gatherers.emplace_back(collision_detector::Gatherer{dog->GetPrevCoordinates(), dog->GetCoordinates(), Dog::COLLISION_RADIUS});
        gatherer_id_to_dog[gatherers.size() - 1] = dog;
    }


    // add item
    enum class ItemType {
        LOOT,
        OFFICE
     };
    std::unordered_map<size_t, ItemType> item_id_to_type;
    std::unordered_map<size_t, LootObject::Id> item_id_to_loot_id;  

    for (auto& [_, loot_obj] : loot_obj_id_to_obj_) {
        items.emplace_back(collision_detector::Item{loot_obj_id_to_coords_.at(loot_obj.GetId()), LootObject::COLLISION_RADIUS});
        auto id = items.size() - 1;
        item_id_to_type[id] = ItemType::LOOT;
        item_id_to_loot_id.emplace(id, loot_obj.GetId());
    }
    for (const auto& office : map_->GetOffices()) {
        geom::Vec2D pos(
            static_cast<double>(office.GetPosition().x),
            static_cast<double>(office.GetPosition().y)
        );
        items.emplace_back(collision_detector::Item{pos, Office::COLLISION_RADIUS});
        auto id = items.size() - 1;        
        item_id_to_type[id] = ItemType::OFFICE;
    }

    // create provider
    VectorItemGathererProvider g_provider{std::move(items),std::move(gatherers)}; 

    auto events = FindGatherEvents(g_provider);
    for (const GatheringEvent& event : events) {
        if (item_id_to_type.at(event.item_id) == ItemType::LOOT) {
            HandleLootCollection(gatherer_id_to_dog.at(event.gatherer_id), item_id_to_loot_id.at(event.item_id));
        } else if (item_id_to_type.at(event.item_id) == ItemType::OFFICE) {
            HandleLootDrop(gatherer_id_to_dog.at(event.gatherer_id));
        }
    }

}

void GameSession::HandleLootCollection(DogPtr dog, LootObject::Id id) {
    if (dog->LootCountInBag() >= map_->GetDogBagCapacity()) {
        return;
    }
    if (auto loot_obj = ExtractLootObject(id)) {
        dog->AddLoot(std::move(*loot_obj));
    }
}

void GameSession::HandleLootDrop(DogPtr dog) {
    dog->DropBag();
}

std::optional<LootObject> GameSession::ExtractLootObject(LootObject::Id id) {
    if (loot_obj_id_to_obj_.contains(id)) {
        auto nh = loot_obj_id_to_obj_.extract(id);
        loot_obj_id_to_coords_.erase(id);
        return std::move(nh.mapped());        
    }
    return std::nullopt;
}

void GameSession::Move(Dog& dog, std::chrono::milliseconds time_delta) const {
    size_t tick = time_delta.count();
    dog.AddTick(tick);
    if (dog.IsStoped()) {
        return;
    }    
    geom::Vec2D dp{dog.GetSpeed().x * tick / 1000,dog.GetSpeed().y * tick / 1000};
    const auto direction = dog.GetDirection();
    auto road_coords = RoundRoadCoords(dog.GetCoordinates());
    auto roads = coords_to_roads_.at(road_coords.x).equal_range(road_coords.y);
    
    double best_dist = 0;
    for (auto it = roads.first; it != roads.second; ++it) {
        double dist = PossibleMoveDist(dog.GetCoordinates(), *it->second, direction);
        if (std::abs(dist) > std::abs(best_dist)) {
            best_dist = dist;
        }
    }
    bool border = (direction == Dog::Direction::WEST || direction == Dog::Direction::EAST)
        ? std::abs(best_dist) <= std::abs(dp.x)
        : std::abs(best_dist) <= std::abs(dp.y);
    if (border) {
        if (direction == Dog::Direction::WEST || direction == Dog::Direction::EAST) {
            dp.x = best_dist;
        }
        if (direction == Dog::Direction::NORTH || direction == Dog::Direction::SOUTH) {
            dp.y = best_dist;
        }
        dog.Stop();
    }

    dog.SetCoordinates(dog.GetCoordinates() + dp);
}


GameSession::DynamicStateContent GameSession::GetDynamicStateContent() const {
    DynamicStateContent::LootObjects loot_objects;
    loot_objects.reserve(loot_obj_id_to_obj_.size());
    for (const auto& [_, obj] : loot_obj_id_to_obj_) {
        loot_objects.emplace_back(obj, loot_obj_id_to_coords_.at(obj.GetId()));
    }
    std::list<Dog> dog_objects;
    for (const auto& dog : dogs_) {
        if (dog)
            dog_objects.emplace_back(*dog);
    }   
    return DynamicStateContent{
        .map_id = map_->GetId(),
        .session_id = id_,
        .dogs = std::move(dog_objects),
        .loot_objects = std::move(loot_objects),
        .dogs_join = dogs_join_,
        .objects_spawned = objects_spawned_
    };   
}

}  // namespace model

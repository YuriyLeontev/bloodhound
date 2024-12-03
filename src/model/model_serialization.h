#pragma once
#include "model.h"
#include "../service/service.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/signals2.hpp>

#include <filesystem>
#include <fstream>


namespace geom {

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

using boost::archive::text_oarchive;
using boost::archive::text_iarchive;

void serialize(text_oarchive& ar, LootObject& obj, [[maybe_unused]] const unsigned version);

void serialize(text_iarchive& ar, LootObject& obj, [[maybe_unused]] const unsigned version);

void serialize(text_oarchive& ar, Dog& dog, [[maybe_unused]] const unsigned version);

void serialize(text_iarchive& ar, Dog& dog, [[maybe_unused]] const unsigned version);

template <typename Archive>
void serialize(Archive& ar, GameSession::DynamicStateContent& content, [[maybe_unused]] const unsigned version) {
    ar & *content.map_id;
    ar & *content.session_id;
    ar & content.dogs;
    ar & content.loot_objects;
    ar & content.dogs_join;
    ar & content.objects_spawned;
}

}  // namespace model

namespace service{

template <typename Archive>
void serialize(Archive& ar, PlayerDynamicStateContent& content, [[maybe_unused]] const unsigned version) {
    ar & *content.token;
    ar & *content.map_id;
    ar & *content.session_id;
    ar & *content.dog_id;
}

} //namespace service

namespace serialization {

namespace sig = boost::signals2;

// LootObjRepr - сериализованное представление класса LootObj
class LootObjRepr {
public:
    LootObjRepr() = default;
    explicit LootObjRepr(const model::LootObject& obj);

    [[nodiscard]] model::LootObject Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & *id_;
        ar & type_;
        ar & worth_;
    }

private:
    model::LootObject::Id id_{0u};
    size_t type_{};
    size_t worth_{};
};

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog);
    [[nodiscard]] model::Dog Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& name_;
        ar& direction_;
        ar& coords_;
        ar& speed_;
        ar& prev_coords_;
        ar& bag_;
        ar& scores_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    model::Dog::Direction direction_;
    geom::Vec2D coords_;
    geom::Vec2D speed_;
    geom::Vec2D prev_coords_;
    model::Dog::Bag bag_;
    size_t scores_ = 0;
};



// ServiceSerializator
class ServiceSerializator {
public:
    ServiceSerializator(service::Service& service, model::Game& game, 
            const std::string path, bool save_require);

    void AutoSave(std::chrono::milliseconds period);

    void Serialize() const;

    void Restore();

private:
    using ServiceState = std::pair<service::PlayersState, model::Game::GameState>;
    service::Service& service_;
    model::Game& game_;
    std::filesystem::path target_file_path_;
    std::filesystem::path buf_file_path_;
    bool has_file_;

    std::chrono::milliseconds counter{0};
    std::chrono::milliseconds save_period;
    sig::scoped_connection tick_service;
};


}  // namespace serialization

#include "model_serialization.h"

namespace model {

template<typename ObjT, typename ReprObjT>
void serialize(text_oarchive& ar, ObjT& obj, [[maybe_unused]] const unsigned version) {
    ReprObjT repr(obj);
    ar << repr;
}

template<typename ObjT, typename ReprObjT>
void serialize(text_iarchive& ar, ObjT& obj, [[maybe_unused]] const unsigned version) {
    ReprObjT repr(obj);
    ar >> repr;
    obj = std::move(repr.Restore());
}

void serialize(text_oarchive& ar, LootObject& obj, [[maybe_unused]] const unsigned version) {
    serialize<LootObject, serialization::LootObjRepr>(ar, obj, version);
}

void serialize(text_iarchive& ar, LootObject& obj, [[maybe_unused]] const unsigned version) {
    serialize<LootObject, serialization::LootObjRepr>(ar, obj, version);
}

void serialize(text_oarchive& ar, Dog& dog, [[maybe_unused]] const unsigned version) {
    serialize<Dog, serialization::DogRepr>(ar, dog, version);
}

void serialize(text_iarchive& ar, Dog& dog, [[maybe_unused]] const unsigned version) {
    serialize<Dog, serialization::DogRepr>(ar, dog, version);
}

}  // namespace model


namespace serialization {

// LootObjRepr -сериализованное представление класса LootObj
LootObjRepr::LootObjRepr(const model::LootObject& obj)
    : id_{obj.GetId()}
    , type_{obj.GetType()}
    , worth_{obj.GetWorth()} {}

[[nodiscard]] model::LootObject LootObjRepr::Restore() const {
    return {id_, type_, worth_};
}

// DogRepr - сериализованное представление класса Dog
DogRepr::DogRepr(const model::Dog& dog)
    : id_(dog.GetId())
    , name_(dog.GetName())
    , direction_(dog.GetDirection())
    , coords_(dog.GetCoordinates())
    , speed_(dog.GetSpeed())
    , prev_coords_(dog.GetPrevCoordinates())
    , bag_(dog.GetBag())
    , scores_(dog.GetScores())  {
}

[[nodiscard]] model::Dog DogRepr::Restore() const {
    model::Dog dog{id_, std::move(name_), prev_coords_, direction_, speed_};
    dog.SetCoordinates(coords_);
    dog.SetScores(scores_);
    for (const auto& item : bag_) {
        dog.AddLoot(item);
    }
    return dog;
}


// ServiceSerializator
ServiceSerializator::ServiceSerializator(service::Service& service, model::Game& game, 
                const std::string path, bool save_require)
    : service_{service}
    , game_{game}
    , target_file_path_{path}
    , has_file_{save_require}{
    buf_file_path_ = target_file_path_;
    buf_file_path_.replace_filename(target_file_path_.stem().string().append("_buf"));
}

void  ServiceSerializator::AutoSave(std::chrono::milliseconds period){
    save_period = period;

    tick_service = service_.DoOnTick([this](std::chrono::milliseconds tick) mutable {
            counter += tick;
            if (counter >= save_period) {
                counter = counter - save_period;
                Serialize();
            }
        });
}

void ServiceSerializator::Serialize() const {
    if (!has_file_) {
        return;
    }
    std::ofstream ss(buf_file_path_);
    boost::archive::text_oarchive oa{ss};
    ServiceState state(service_.GetPlayersState(), game_.GetGameState());
    oa << state;
    std::filesystem::rename(buf_file_path_, target_file_path_);
}

void ServiceSerializator::Restore() {
    if (!has_file_) {
        return;
    }
    if (std::error_code ec; !std::filesystem::exists(target_file_path_, ec)) {
        return;
    }    
    std::ifstream ss(target_file_path_);
    boost::archive::text_iarchive ia{ss};
    ServiceState state;
    ia >> state;

    auto& [players_state, game_state] = state;
    for (auto& session_state : game_state) {
        model::GameSession* session = game_.AddGameSession(
            session_state.map_id,
            *session_state.session_id,
            session_state.dogs_join,
            session_state.objects_spawned
        );
        for (model::Dog& dog : session_state.dogs) {
            session->AddDog(std::move(dog));
        }
        for (auto& [obj, coords] : session_state.loot_objects) {
            session->AddLoot(obj, coords);
        }
    }
    for (auto& player_state : players_state) {
        service_.AddPlayer(std::move(player_state.token), player_state.map_id, player_state.session_id, player_state.dog_id);
    }    
}

}
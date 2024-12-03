#include "service.h"

namespace service {

// UseCaseBase
UseCaseBase::UseCaseBase(Service* service)
    : service_{service} {}

model::Game& UseCaseBase::GetGame() const noexcept{
    return service_->game_;
}

Players& UseCaseBase::GetPlayers() const noexcept{
    return service_->players_;
}

PlayerTokens& UseCaseBase::GetPlayerTokens() const noexcept{
    return service_->player_tokens_;
}

repository::SaveScoresFactory& UseCaseBase::GetSaveScoresFactory(){
    return service_->db_.GetSaveScoresFactory();
}

UseCaseJoinPlayer::Result UseCaseJoinPlayer::operator()(const model::Map::Id& map_id, std::string dog_name) {
    model::GameSession* session = GetGame().GetGameSessionByMapId(map_id);
    if (!session) {
        return std::nullopt;
    }
    auto dog = session->NewDog(std::move(dog_name));
    auto player = GetPlayers().AddPlayer(dog, session);
    Token token = GetPlayerTokens().AddPlayer(player);
    return std::make_pair(std::move(token), player->GetDog().GetId());
}

UseCaseGetPlayers::Result UseCaseGetPlayers::operator()(const Token& player_token) {
    UseCaseGetPlayers::Result result = std::nullopt;
    if (auto player = GetPlayerTokens().FindPlayerByToken(player_token)) {
        const auto& dogs = player->GetGameSession().GetDogs();
        result.emplace().reserve(dogs.size());
        for (const auto& dog : dogs) {
            result->emplace_back(dog->GetId(), dog->GetName());
        }
    }
    return result;
}

UseCaseGetGameState::Result UseCaseGetGameState::operator()(const Token& player_token) {
    UseCaseGetGameState::Result result = std::nullopt;

    if (auto player = GetPlayerTokens().FindPlayerByToken(player_token)) {
        const auto& dogs = player->GetGameSession().GetDogs();
        result.emplace().players.reserve(dogs.size());
        for (const auto& dog : dogs) {
            UseCaseGetGameState::PlayerState::Bag player_bag;
            for (const auto& loot_item : dog->GetBag()) {
                player_bag.emplace_back(*loot_item.GetId(), loot_item.GetType());
            }
            result->players.emplace_back( dog->GetId(), 
                                        dog->GetCoordinates(), 
                                        dog->GetSpeed(),
                                        dog->GetDirection(), 
                                        std::move(player_bag),
                                        dog->GetScores());
        }
        
        const auto& session = player->GetGameSession();
        const auto& loot_oblects = session.GetLootObjects();
        result->loot_objects.reserve(loot_oblects.size());
        for (const auto& [obj_id, obj] : loot_oblects ) {
            result->loot_objects.emplace_back(obj_id, obj.GetType(), session.GetLootCoordsById(obj_id));
        }
    }    
    return result;
}

bool UseCaseGameAction::operator()(const Token& player_token, model::Dog::Direction dir) {
    if (auto player = GetPlayerTokens().FindPlayerByToken(player_token)) {
        if (dir == model::Dog::Direction::STOP){
            player->GetDog().Stop();
            return true;
        }        
        const auto speed = player->GetGameSession().GetMap().GetDogSpeed();
        player->GetDog().SetDirection(dir);
        player->GetDog().SetSpeed(speed);
        return true;
    }      
    return false;
}

bool UseCaseTimeTick::operator()(std::chrono::milliseconds time_delta) { 
    if (service_->GetTimeTicker()){
        return false;    
    }
    service_->Tick(time_delta);
    return true;
}

std::vector<RetiredPlayer> UseCaseRecords::operator()(int offset, int limit) {
    auto unit = GetSaveScoresFactory().CreateSaveScores();
    return unit->PlayerRepository().GetSavedRetiredPlayers(offset, limit);
}


bool UseCaseDogRetire::operator()(model::Dog::Id dog_id, const model::Map::Id& map_id) {
    using namespace std::chrono;
    auto unit = GetSaveScoresFactory().CreateSaveScores();
    auto player = GetPlayers().FindByDogIdAndMapId(dog_id, map_id);
    model::Dog& dog = player->GetDog();
    unit->PlayerRepository().Save({RetiredPlayerId::New(), dog.GetName(), dog.GetScores(), dog.GetTimeInGame()});
    unit->Commit();
    GetPlayers().ErasePlayer(dog_id, map_id);
    GetPlayerTokens().ErasePlayer(player);
    return true;
}

Service::Service(model::Game& game, repository::Database & db)
    : game_(game)
    , db_(db) 
    , JoinPlayer{this}
    , GetPlayers{this}
    , GetGameState{this}
    , GameAction{this}
    , TimeTick{this}
    , Records{this}
    , DogRetire{this} {
    dog_retire_listener = game_.RetireListener([this](model::Dog::Id dog, const model::Map::Id& map) {this->DogRetire(dog, map);});
}

const model::Game::Maps& Service::GetMaps() const noexcept{
    return game_.GetMaps();
}

const model::Map* Service::FindMap(const model::Map::Id& id) const noexcept{
    return game_.FindMap(id);
}

void Service::OnTimeTicker(){
    time_ticker_ = true;
}

bool Service::GetTimeTicker(){
    return time_ticker_;
}

void Service::Tick(std::chrono::milliseconds time_delta){
    game_.OnTick(time_delta);

    // Уведомляем подписчиков сигнала tick
    tick_signal_(time_delta);  
}

PlayersState Service::GetPlayersState() const {
    return player_tokens_.GetPlayersState();
}

void Service::AddPlayer(Token token, const model::Map::Id& map_id,
    model::GameSession::Id session_id, model::Dog::Id dog_id) {
    model::GameSession* session =  game_.GetGameSessionByMapId(map_id);
    if (!session || session->GetId() != session_id) {
        throw std::runtime_error("Game session not found");
    }
    auto dog = session->GetDogById(dog_id);
    if (!dog) {
        throw std::runtime_error("Dog not found");
    }
    auto player = players_.AddPlayer(dog, session);
    player_tokens_.AddPlayer(player, std::move(token));
}

}
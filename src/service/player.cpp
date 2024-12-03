#include "player.h"

#include <sstream>
#include <iomanip>

namespace detail {

// TokenGenerator
std::string TokenGenerator::operator()() {
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('f') << generator1_()
        << std::setw(16) << std::setfill('a') << generator2_();
    return ss.str();
}

} //namespace detail



namespace service {

// Player
Player::Player(std::shared_ptr<model::Dog> dog, model::GameSession* session)
    : dog_{dog}
    , session_{session} {}

model::Dog& Player::GetDog() const noexcept {
    return *dog_;
}

model::GameSession& Player::GetGameSession() const noexcept {
    return *session_;
}


// Players
std::shared_ptr<Player> Players::AddPlayer(std::shared_ptr<model::Dog> dog, model::GameSession* session) {
    auto [it, inserted] = players_.emplace(
        std::make_pair(dog->GetId(), session->GetMap().GetId()),
        std::make_shared<Player>(dog, session)
    );
    if (!inserted) {
        throw std::runtime_error("Dog already exists");
    }
    return it->second;
}

std::shared_ptr<Player> Players::FindByDogIdAndMapId(model::Dog::Id dog_id, const model::Map::Id& map_id) {
    if (auto it = players_.find({dog_id, map_id}); it != players_.end()) {
        return it->second;
    }
    return nullptr;
}

void Players::ErasePlayer(model::Dog::Id dog_id, const model::Map::Id& map_id) {
    players_.erase({dog_id, map_id});
}

size_t Players::Hasher::operator()(const std::pair<model::Dog::Id, model::Map::Id>& item) const {
    return util::TaggedHasher<model::Dog::Id>{}(item.first) ^ util::TaggedHasher<model::Map::Id>{}(item.second);
}

// PlayerTokens
const Token& PlayerTokens::AddPlayer(const std::shared_ptr<Player> player) {
    auto [it, inserted] = token_to_player_.emplace(Token{get_token_()}, player);
    while (!inserted) {
        std:tie(it, inserted) = token_to_player_.emplace(Token{get_token_()}, player);
    }
    player_to_token_.emplace(player, &it->first);
    return it->first;
}

void PlayerTokens::AddPlayer(const std::shared_ptr<Player> player, Token token) {
    
    auto [it, inserted] = token_to_player_.emplace(std::move(token), player);
    if (!inserted) {
        throw std::runtime_error("Player already exists");
    }
    player_to_token_.emplace(player, &it->first);
}

PlayersState PlayerTokens::GetPlayersState() const {
    PlayersState content;
    for (auto [token, player] : token_to_player_) {
        content.emplace_back(
            std::move(token),
            player->GetGameSession().GetMap().GetId(),
            player->GetGameSession().GetId(),
            player->GetDog().GetId()
        );
    }
    return content;
}

void PlayerTokens::ErasePlayer(std::shared_ptr<Player> player) {
    const Token* token = player_to_token_.at(player);
    token_to_player_.erase(*token);
    player_to_token_.erase(player);
}

std::shared_ptr<Player> PlayerTokens::FindPlayerByToken(const Token& token) const {
    if (token_to_player_.contains(token)) {
        return token_to_player_.at(token);
    }
    return nullptr;
}

RetiredPlayer::RetiredPlayer(RetiredPlayerId id, std::string name, size_t score, size_t play_time)
    : id_(std::move(id))
    , name_(std::move(name))
    , score_(score)
    , play_time_(play_time) {
}

const RetiredPlayerId& RetiredPlayer::GetId() const noexcept {
    return id_;
}

const std::string& RetiredPlayer::GetName() const noexcept {
    return name_;
}

size_t RetiredPlayer::GetScore() const noexcept {
    return score_;
}

size_t RetiredPlayer::PlayTime() const noexcept {
    return play_time_;
}

}
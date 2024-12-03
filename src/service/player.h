#pragma once

#include "../model/model.h"
#include "../util/tagged_uuid.h"

#include <random>
#include <unordered_map>

namespace detail {

struct TokenTag {};

class TokenGenerator {
public:
    std::string operator()();
private:
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
};

struct RetiredPlayerTag {};

}  // namespace detail

namespace service {

using Token = util::Tagged<std::string, detail::TokenTag>;

class Player {
public:
    explicit Player(std::shared_ptr<model::Dog> dog, model::GameSession* session);

    model::Dog& GetDog() const noexcept;

    model::GameSession& GetGameSession() const noexcept;

private:
    std::shared_ptr<model::Dog> dog_ = nullptr;
    model::GameSession* session_ = nullptr;
};

struct PlayerDynamicStateContent {
    Token token{""};
    model::Map::Id map_id{""};
    model::GameSession::Id session_id{0u};
    model::Dog::Id dog_id{0u};
}; 
using PlayersState = std::vector<PlayerDynamicStateContent>;

class Players {
public:
    std::shared_ptr<Player> AddPlayer(std::shared_ptr<model::Dog> dog, model::GameSession* session);
    std::shared_ptr<Player> FindByDogIdAndMapId(model::Dog::Id dog_id, const model::Map::Id& map_id);

    void ErasePlayer(model::Dog::Id dog_id, const model::Map::Id& map_id);
private:

    struct Hasher{size_t operator()(const std::pair<model::Dog::Id, model::Map::Id>& item) const;};

    using DogIdAndMapIdToPlayer = std::unordered_map<std::pair<model::Dog::Id, model::Map::Id>, std::shared_ptr<Player>, Hasher>;
    DogIdAndMapIdToPlayer players_;
};

class PlayerTokens {
public:
    const Token& AddPlayer(const std::shared_ptr<Player> player);
    void AddPlayer(const std::shared_ptr<Player> player, Token token);
    std::shared_ptr<Player> FindPlayerByToken(const Token& token) const;

    PlayersState GetPlayersState() const;

    void ErasePlayer(std::shared_ptr<Player> player);

private:
    std::unordered_map<Token, std::shared_ptr<Player>, util::TaggedHasher<Token>> token_to_player_;
    std::unordered_map<std::shared_ptr<Player>, const Token*> player_to_token_;
    detail::TokenGenerator get_token_;
};

using RetiredPlayerId = util::TaggedUUID<detail::RetiredPlayerTag>;

class RetiredPlayer {
public:
    RetiredPlayer(RetiredPlayerId id, std::string name, size_t score, size_t play_time);

    const RetiredPlayerId& GetId() const noexcept;

    const std::string& GetName() const noexcept;

    size_t GetScore() const noexcept;

    size_t PlayTime() const noexcept;

private:
    RetiredPlayerId id_;
    std::string name_;
    size_t score_;
    size_t play_time_;
};

class RetiredPlayerRepository {
public:
    virtual void Save(const RetiredPlayer& player) = 0;

    virtual std::vector<RetiredPlayer> GetSavedRetiredPlayers(int offset, int limit) = 0;

protected:
    ~RetiredPlayerRepository() = default;
};


}
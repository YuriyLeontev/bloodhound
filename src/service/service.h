
#pragma once

#include "../model/model.h"
#include "../model/geom.h"
#include "player.h"
#include "../repository/repository.h"

#include <optional>
#include <chrono>

#include <boost/signals2.hpp>

namespace service {

class Service;
class UseCaseBase {
public:
    explicit UseCaseBase(Service* service);

protected:
    model::Game& GetGame() const noexcept;
    Players& GetPlayers() const noexcept;
    PlayerTokens& GetPlayerTokens() const noexcept;
    Service* service_;
    repository::SaveScoresFactory& GetSaveScoresFactory();
};

class UseCaseJoinPlayer : public UseCaseBase {
    using UseCaseBase::UseCaseBase;
public:
    using Result = std::optional<std::pair<Token, model::Dog::Id>>;
    Result operator()(const model::Map::Id& map_id, std::string dog_name);
};

class UseCaseGetPlayers : public UseCaseBase {
    using UseCaseBase::UseCaseBase;
public:
    using Result = std::optional<std::vector<std::pair<model::Dog::Id, std::string_view>>>;
    Result operator()(const Token& player_token);
};

class UseCaseGetGameState : public UseCaseBase {
    using UseCaseBase::UseCaseBase;
public:
    struct PlayerState {
        struct LootObj {
            size_t id, type;
        };        
        model::Dog::Id id;
        geom::Vec2D pos;
        geom::Vec2D speed;
        model::Dog::Direction dir;
        using Bag = std::vector<LootObj>;
        Bag bag;       
        size_t scores;
    };

    struct LootObjectState {
        model::LootObject::Id id;
        size_t type;
        geom::Vec2D pos;
    };
    struct GameState {
        std::vector<PlayerState> players;
        std::vector<LootObjectState> loot_objects;
    };
    using Result = std::optional<GameState>;
    Result operator()(const Token& player_token);
};

class UseCaseGameAction : public UseCaseBase {
    using UseCaseBase::UseCaseBase;
public:
    bool operator()(const Token& player_token, model::Dog::Direction dir);
};

class UseCaseTimeTick : public UseCaseBase {
    using UseCaseBase::UseCaseBase;
public:
    bool operator()(std::chrono::milliseconds time_delta);
};

class UseCaseRecords : public UseCaseBase {
public:
    using UseCaseBase::UseCaseBase;
    std::vector<RetiredPlayer> operator()(int offset, int limit);
};

class UseCaseDogRetire : public UseCaseBase {
public:
    using UseCaseBase::UseCaseBase;
    bool operator()(model::Dog::Id dog, const model::Map::Id&);
};

// Service
class Service {
public:
    explicit Service(model::Game& game, repository::Database& db);

    const model::Game::Maps& GetMaps() const noexcept;
    const model::Map* FindMap(const model::Map::Id& id) const noexcept;

    void OnTimeTicker();
    bool GetTimeTicker();

    void Tick(std::chrono::milliseconds time_delta);

    // Добавляем обработчик сигнала tick и возвращаем объект connection для управления,
    // при помощи которого можно отписаться от сигнала
    using TickSignal = boost::signals2::signal<void(std::chrono::milliseconds delta)>;
    [[nodiscard]] boost::signals2::connection DoOnTick(const TickSignal::slot_type& handler) {
        return tick_signal_.connect(handler);
    }

    void AddPlayer(Token token, const model::Map::Id& map_id,
        model::GameSession::Id session_id, model::Dog::Id dog_id);

    PlayersState GetPlayersState() const;

    UseCaseJoinPlayer   JoinPlayer;
    UseCaseGetPlayers   GetPlayers;
    UseCaseGetGameState GetGameState;
    UseCaseGameAction   GameAction;
    UseCaseTimeTick     TimeTick;
    UseCaseRecords      Records;
    UseCaseDogRetire    DogRetire;


private:
    friend UseCaseBase;

    model::Game& game_;
    Players players_;
    PlayerTokens player_tokens_;
    repository::Database& db_;

    bool time_ticker_ = false;
    
    TickSignal tick_signal_;

    boost::signals2::scoped_connection dog_retire_listener;
};

}
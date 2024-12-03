#include "postgres.h"

#include <pqxx/pqxx>

#include <string>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

// DatabasePostgres
DatabasePostgres::DatabasePostgres(size_t thread_num, const std::string& db_url)
    : unit_factory_{thread_num, db_url} {
    pqxx::connection conn(db_url);
    pqxx::work work(conn);
    work.exec(
        R"(CREATE TABLE IF NOT EXISTS retired_players (
            id UUID CONSTRAINT firstindex PRIMARY KEY,
            name varchar(100) NOT NULL,
            score INT NOT NULL,
            play_time_ms INT NOT NULL
        );
        CREATE UNIQUE INDEX IF NOT EXISTS
            score_play_time_idx
        ON
        retired_players (score DESC,
        play_time_ms,
        name);
        )"_zv
    );
    work.commit();
}

// SaveScoresFactoryPostgres::
SaveScoresFactoryPostgres::SaveScoresFactoryPostgres(size_t thread_num, const std::string& db_url)
    : conn_pool_{thread_num, [db_url] {return std::make_shared<pqxx::connection>(db_url);}} {
    }

std::unique_ptr<service::SaveScores> SaveScoresFactoryPostgres::CreateSaveScores() {
    return std::make_unique<SaveScoresPostgres>(conn_pool_.GetConnection());
}


// SaveScoresPostgres::
SaveScoresPostgres::SaveScoresPostgres(conn_pool::ConnectionPool::ConnectionWrapper&& connection)
: connection_{std::move(connection)}
, work_{*connection_} {
}

service::RetiredPlayerRepository& SaveScoresPostgres::PlayerRepository() {
    return player_rep_;
}

void SaveScoresPostgres::Commit() {
    work_.commit();
}

// RetiredPlayerRepoPostgres
RetiredPlayerRepoPostgres::RetiredPlayerRepoPostgres(pqxx::work& work)
    : work_{work} {
}

void RetiredPlayerRepoPostgres::Save(const service::RetiredPlayer& player) {
    work_.exec_params(
        "INSERT INTO retired_players (id, name, score, play_time_ms) "
        "VALUES ($1, $2, $3, $4) "_zv,
        player.GetId().ToString(), player.GetName(), player.GetScore(), player.PlayTime());
}

std::vector<service::RetiredPlayer> RetiredPlayerRepoPostgres::GetSavedRetiredPlayers(int offset, int limit) {
    std::vector<service::RetiredPlayer> players;
    std::ostringstream query_text;
    query_text
        << "SELECT id, name, score, play_time_ms FROM retired_players "sv
        << "ORDER BY score DESC, play_time_ms, name LIMIT "sv << limit << " OFFSET "sv << offset << ";"sv;
    // Выполняем запрос и итерируемся по строкам ответа
    for (auto [id, name, score, play_time] : work_.query<std::string, std::string, int, int>(query_text.str())) {
        players.emplace_back(service::RetiredPlayerId::FromString(id), std::move(name), score, play_time);
    }
    return players;
}

} //namespace postgres
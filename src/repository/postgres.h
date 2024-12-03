#pragma once

#include "repository.h"
#include "connection_pool.h"

namespace postgres {


class RetiredPlayerRepoPostgres : public service::RetiredPlayerRepository {
public:
    explicit RetiredPlayerRepoPostgres(pqxx::work& work);

    void Save(const service::RetiredPlayer& player) override;

    std::vector<service::RetiredPlayer> GetSavedRetiredPlayers(int offset, int count) override;

private:
    pqxx::work& work_;
};

class SaveScoresPostgres : public service::SaveScores{
public:
    explicit SaveScoresPostgres(conn_pool::ConnectionPool::ConnectionWrapper&& connection);

    service::RetiredPlayerRepository& PlayerRepository() override;

    void Commit() override;

private:
    conn_pool::ConnectionPool::ConnectionWrapper&& connection_;
    pqxx::work work_;
    RetiredPlayerRepoPostgres player_rep_{work_};
};

class SaveScoresFactoryPostgres : public repository::SaveScoresFactory {
public:
    explicit SaveScoresFactoryPostgres(size_t thread_num, const std::string& db_url);

    std::unique_ptr<service::SaveScores> CreateSaveScores() override;
private:
    conn_pool::ConnectionPool conn_pool_;
};

class DatabasePostgres : public repository::Database {
public:
    DatabasePostgres(size_t thread_num, const std::string& db_url);

    repository::SaveScoresFactory& GetSaveScoresFactory() override {
        return unit_factory_;
    }

private:
    SaveScoresFactoryPostgres unit_factory_;    
};

} // namespace postgres
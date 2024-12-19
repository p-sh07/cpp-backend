//
// Created by Pavel on 19.12.2024.
//
#pragma once;
#include "database.h"
namespace postgres {


class Database final : public database::DatabaseInterface {
public:
    Database(const std::string& database_url, int max_conections = 1)
    //make a connection pool using a lambda as a connection factory
        : conn_pool_(max_conections, [&database_url]() {
            return std::make_shared<pqxx::connection>(database_url);
        }) {
        auto conn = conn_pool_.GetConnection();
        //TODO: create table, etc;
    }

    void SavePlayersStats(const std::vector<gamedata::PlayerStats>& players_stats) override {
        //TODO:
    }
    virtual std::vector<gamedata::PlayerStats> LoadPlayersStats() const override {
        //TODO:
    }

private:
    database::ConnectionPool conn_pool_;
};

} //namespace postgres
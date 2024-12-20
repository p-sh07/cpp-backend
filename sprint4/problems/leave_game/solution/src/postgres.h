//
// Created by Pavel on 19.12.2024.
//
#pragma once;
#include "database.h"

namespace postgres {
using pqxx::operator"" _zv;

class PlayerStatsImpl final : public database::PlayerStats {
public:
    PlayerStatsImpl(const std::string& database_url, int max_conections = 1)
    //make a connection pool using a lambda as a connection factory
    ;

    void SavePlayersStats(const std::vector<gamedata::PlayerStats>& players_stats) override;
    std::vector<gamedata::PlayerStats> LoadPlayersStats(std::optional<size_t> start, std::optional<size_t> maxItems) override;

private:
    static constexpr size_t max_allowed_results_ = 100;
    database::ConnectionPool conn_pool_;
};

} //namespace postgres
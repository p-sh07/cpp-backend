//
// Created by Pavel on 19.12.2024.
//

#include "postgres.h"

namespace postgres {
PlayerStatsImpl::PlayerStatsImpl(const std::string& database_url, int max_conections)
//make a connection pool using a lambda as a connection factory
    : conn_pool_(max_conections, [&database_url]() {
        return std::make_shared<pqxx::connection>(database_url);
    }) {
    auto conn = conn_pool_.GetConnection();
    //TODO: create table, etc;
    pqxx::work wk{*conn};
    wk.exec(R"(CREATE TABLE IF NOT EXISTS retired_players (
                id SERIAL PRIMARY KEY,
                name varchar(40) NOT NULL,
                score integer CONSTRAINT score_positive CHECK (score >= 0),
                game_time integer NOT NULL CONSTRAINT game_time_positive CHECK (game_time >= 0)
            );
            CREATE INDEX IF NOT EXISTS score_index ON retired_players (score);
        )"_zv);
    wk.commit();
}

void PlayerStatsImpl::SavePlayersStats(const std::vector<gamedata::PlayerStats>& players_stats) {
    //do not get a connection if nothing to write
    if(players_stats.empty()) {
        return;
    }

    auto conn = conn_pool_.GetConnection();
    pqxx::work work_{*conn};
    for (const auto& pstat : players_stats) {
        work_.exec_params(R"(
            INSERT INTO retired_players (name, score, game_time) VALUES ($1, $2, $3);
            )"_zv, pstat.name, pstat.score, pstat.game_time_msec
        );
    }
    work_.commit();
}

std::vector<gamedata::PlayerStats> PlayerStatsImpl::LoadPlayersStats(std::optional<size_t> start, std::optional<size_t> maxItems) {
    std::vector<gamedata::PlayerStats> result;
    auto conn = conn_pool_.GetConnection();
    pqxx::read_transaction rd{*conn};
    std::string query_str = "SELECT name, score, game_time FROM retired_players ORDER BY score DESC, game_time ASC, name ASC LIMIT "
                            + std::to_string(maxItems ? *maxItems : max_allowed_results_) + " OFFSET " + std::to_string(start ? *start : 0u) + ";";

    for(auto const&[name, score, game_time_msec] : rd.query<std::string, size_t, size_t>(query_str)) {
        result.emplace_back(name, score, game_time_msec);
    }
    return result;
}
} //namespace postgres
#pragma once
//define this for std::string_view use in json, comment for now - doesn't compile
//#define BOOST_JSON_STANDALONE

#include <boost/json.hpp>
#include <filesystem>

#include "application.h"

namespace json_loader {
namespace json = boost::json;

struct JsonKeys {
    using Key = std::string_view;

    static constexpr Key maps = "maps";
    static constexpr Key roads = "roads";
    static constexpr Key buildings = "buildings";
    static constexpr Key offices = "offices";
    static constexpr Key loot_gen = "lootGeneratorConfig";
    static constexpr Key loot_types = "lootTypes";
    static constexpr Key dog_speed = "dogSpeed";
    static constexpr Key bag_cap = "bagCapacity";

    static constexpr Key dog_speed_dflt = "defaultDogSpeed";
    static constexpr Key bag_cap_dflt = "defaultBagCapacity";
};

json::value MapToValue(const model::Map& map);

std::string PrintMap(const model::Map& map);
std::string PrintMapList(const model::Game::Maps& map_list);

const char ParseMove(const std::string& request_body);
model::TimeMs ParseTick(const std::string& request_body);

struct StatsCommand {
    std::optional<size_t> start {std::nullopt};
    std::optional<size_t> maxItems {std::nullopt};
};

StatsCommand ParseStatsCommand(const std::string& request_body);

std::string PrintPlayerList(const std::vector<app::ConstPlayerPtr>& players);
std::string PrintGameState(app::ConstPlayerPtr& player, const std::shared_ptr<app::GameInterface>& game_app);
std::string PrintPlayerStats(const std::vector<gamedata::PlayerStats>& players_stats);

model::Game LoadGame(const std::filesystem::path& json_path);
} // namespace json_loader


//======= Json tag_invoke overloads ===========
namespace geom {
namespace json = boost::json;

void tag_invoke(const json::value_from_tag&, json::value& jv, Point2D const& pt);
Point2D tag_invoke(const json::value_to_tag<Point2D>&, json::value const& jv);

void tag_invoke(const json::value_from_tag&, json::value& jv, Vec2D const& speed);
Vec2D tag_invoke(const json::value_to_tag<Vec2D>&, json::value const& jv);
}

//tag_invoke needs to be in namespace model for correct ADL behaviour
namespace model {

namespace json = boost::json;
//Point
void tag_invoke(const json::value_from_tag&, json::value& jv, Point const& pt);
Point tag_invoke(const json::value_to_tag<Point>&, json::value const& jv);

//Dir
void tag_invoke(const json::value_from_tag&, json::value& jv, Direction const& dir);
Direction tag_invoke(const json::value_to_tag<Direction>&, json::value const& jv);

//Map overloads
void tag_invoke(const json::value_from_tag&, json::value& jv, Map const& map);
Map tag_invoke(const json::value_to_tag<Map>&, json::value const& jv);

//Road
void tag_invoke(const json::value_from_tag&, json::value& jv, Road const& rd);
Road tag_invoke(const json::value_to_tag<Road>&, json::value const& jv);

//Building
void tag_invoke(const json::value_from_tag&, json::value& jv, Building const& bd);
Building tag_invoke(const json::value_to_tag<Building>&, json::value const& jv);

//Office
void tag_invoke(const json::value_from_tag&, json::value& jv, Office const& offc);
Office tag_invoke(const json::value_to_tag<Office>&, json::value const& jv);

//BagContent Items
void tag_invoke(const json::value_from_tag&, json::value& jv, Dog::BagContent const& bag);

} // namespace model
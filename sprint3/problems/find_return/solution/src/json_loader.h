#pragma once
//define this for std::string_view use in json, comment for now - doesn't compile
//#define BOOST_JSON_STANDALONE

#include <boost/json.hpp>
#include <filesystem>

#include "application.h"

namespace json_loader {
namespace json = boost::json;

struct JsonKeys {
    using Key = boost::string_view;

    static constexpr Key maps = "maps";
    static constexpr Key roads = "roads";
    static constexpr Key buildings = "buildings";
    static constexpr Key offices = "offices";
    static constexpr Key loot_gen = "lootGeneratorConfig";
    static constexpr Key loot_types = "lootTypes";
    static constexpr Key dog_speed = "DogSpeed";
    static constexpr Key bag_cap = "bagCapacity";

    static constexpr Key dog_speed_dflt = "defaultDogSpeed";
    static constexpr Key bag_cap_dflt = "defaultBagCapacity";
};

json::value MapToValue(const model::Map& map);

std::string PrintMap(const model::Map& map);
std::string PrintMapList(const model::Game::Maps& map_list);

const char ParseMove(const std::string& request_body);
model::TimeMs ParseTick(const std::string& request_body);

std::string PrintPlayerList(const std::vector<app::PlayerPtr>& players);
std::string PrintGameState(const app::PlayerPtr player, const std::shared_ptr<app::GameInterface>& game_app);

model::Game LoadGame(const std::filesystem::path& json_path);
} // namespace json_loader

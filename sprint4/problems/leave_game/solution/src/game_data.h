//
// Created by Pavel on 23.09.2024.
//
#pragma once
#include <boost/json.hpp>
#include <chrono>
#include <optional>

namespace gamedata {
using namespace std::literals;

//NB: Default values set here!
struct Settings {
    bool randomised_dog_spawn = false;

    std::optional<double> map_dog_speed;
    std::optional<size_t> map_bag_capacity;

    double loot_gen_prob = 0;
    std::chrono::milliseconds loot_gen_interval{0u};

    double default_dog_speed    = 1.0;
    size_t default_bag_capacity = 3;

    const double loot_item_width = 0.0;
    const double dog_width       = 0.6;
    const double office_width    = 0.5;

    static constexpr auto valid_move_chars = "UDLRN"sv;

    double GetDogSpeed() const {
        return map_dog_speed ? *map_dog_speed : default_dog_speed;
    }

    size_t GetBagCap() const {
        return map_bag_capacity ? *map_bag_capacity : default_bag_capacity;
    }
};

namespace json = boost::json;
using LootItemType = unsigned;

//Funcs -> store map loot info from json
//Get json for printing
class LootTypesInfo {
public:
    //json [array of loot types] }
    explicit LootTypesInfo(json::array map_loot_types);

    json::array AsJsonArray() const;

    size_t Size();

    const size_t Size() const;

    size_t GetItemValue(LootItemType type) const;

private:
    size_t size_ = 0;
    json::array map_loot_types_;
};

//Player stats for database
struct PlayerStats {
    std::string name {""s};
    size_t score {0};
    size_t game_time_msec {0};
};

}

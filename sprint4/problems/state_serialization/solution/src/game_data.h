//
// Created by Pavel on 23.09.2024.
//
#pragma once
#include <boost/json.hpp>
#include <chrono>

namespace gamedata
{
namespace json = boost::json;
using namespace std::literals;

//NB: Default values set here
struct Settings {

    bool randomised_dog_spawn = false;
    double loot_gen_prob = 0.5;
    std::chrono::milliseconds loot_gen_interval{1000u};

    double default_dog_speed    = 1.0;
    size_t default_bag_capacity = 3;
    std::chrono::milliseconds default_inactivity_timeout_ms_ {60000u};

    const double loot_item_width = 0.0;
    const double dog_width       = 0.6;
    const double office_width    = 0.5;

    static constexpr auto valid_move_chars = "UDLRN"sv;
};

//Funcs -> store map loot info from json
//Get json for printing
class LootTypeInfo {
 public:
    //json [array of loot types] }
    explicit LootTypeInfo(json::array map_loot_types);

    json::array AsJsonArray() const;

    size_t Size();
    const size_t Size() const;

    size_t GetItemValue(unsigned type) const {
        auto val = map_loot_types_.at(type).as_object().at("value").as_int64();
        return static_cast<size_t>(val);
    }
 private:
    size_t size_ = 0;
    json::array map_loot_types_;
};

} //namespace gamedata

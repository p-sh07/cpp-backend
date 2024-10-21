//
// Created by Pavel on 23.09.2024.
//
#pragma once
#include <boost/json.hpp>

namespace gamedata
{
namespace json = boost::json;

//Funcs -> store map loot info from json
//Get json for printing
class LootTypeInfo {
 public:
    //json [array of loot types] }
    LootTypeInfo(json::array map_loot_types);

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

}

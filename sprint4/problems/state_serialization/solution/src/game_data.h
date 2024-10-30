//
// Created by Pavel on 23.09.2024.
//
#pragma once
#include <boost/json.hpp>

namespace gamedata
{
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

}

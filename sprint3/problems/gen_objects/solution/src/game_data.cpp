//
// Created by Pavel on 23.09.2024.
//

#include "game_data.h"

boost::json::array gamedata::LootTypeInfo::AsJsonArray() const {
    return map_loot_types_;
}
gamedata::LootTypeInfo::LootTypeInfo(boost::json::array map_loot_types)
    : map_loot_types_(std::move(map_loot_types)) {
}

size_t gamedata::LootTypeInfo::Size() {
    return map_loot_types_.size();
}

const size_t gamedata::LootTypeInfo::Size() const {
    return map_loot_types_.size();
}

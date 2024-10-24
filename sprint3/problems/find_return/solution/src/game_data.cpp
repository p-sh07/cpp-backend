//
// Created by Pavel on 23.09.2024.
//

#include "game_data.h"

namespace gamedata {

boost::json::array LootTypeInfo::AsJsonArray() const {
    return map_loot_types_;
}
LootTypeInfo::LootTypeInfo(boost::json::array map_loot_types)
    : map_loot_types_(std::move(map_loot_types)) {
}

size_t LootTypeInfo::Size() {
    return map_loot_types_.size();
}

const size_t LootTypeInfo::Size() const {
    return map_loot_types_.size();
}

}
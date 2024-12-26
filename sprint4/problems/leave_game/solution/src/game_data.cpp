//
// Created by Pavel on 23.09.2024.
//

#include "game_data.h"

namespace gamedata {

boost::json::array LootTypesInfo::AsJsonArray() const {
    return map_loot_types_;
}
LootTypesInfo::LootTypesInfo(boost::json::array map_loot_types)
    : map_loot_types_(std::move(map_loot_types)) {
}

size_t LootTypesInfo::Size() {
    return map_loot_types_.size();
}

const size_t LootTypesInfo::Size() const {
    return map_loot_types_.size();
}

size_t LootTypesInfo::GetItemValue(LootItemType type) const {
    auto val = map_loot_types_.at(type).as_object().at("value").as_int64();
    return static_cast<size_t>(val);
}

}
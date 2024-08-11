#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "model.h"

namespace json_loader {
    namespace json = boost::json;
    // using json::value_from;
    // using json::value_to;

    // void tag_invoke(json::value_from_tag, json::value &jv,
    //                              model::Map const& map);

    std::string PrintMapList(const model::Game& game);
    std::string PrintMap(const model::Map& map);

    model::Game LoadGame(const std::filesystem::path &json_path);
} // namespace json_loader

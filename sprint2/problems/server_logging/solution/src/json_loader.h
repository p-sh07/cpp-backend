#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "boost_log.h"
#include "model.h"

namespace json = boost::json;

namespace json_loader {

    std::string PrintMap(const model::Map &map);
    std::string PrintMapList(const model::Game &game);
    std::string PrintErrorMsgJson(json::string_view code, json::string_view message);

    model::Game LoadGame(const std::filesystem::path &json_path);
} // namespace json_loader

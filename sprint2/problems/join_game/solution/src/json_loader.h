#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "server_logger.h"
#include "model.h"

namespace json_loader {
namespace json = boost::json;

std::string PrintMap(const model::Map& map);
std::string PrintMapList(const model::Game& game);
std::string PrintErrorMsgJson(json::string_view code, json::string_view message);

model::Game LoadGame(const std::filesystem::path& json_path);
} // namespace json_loader

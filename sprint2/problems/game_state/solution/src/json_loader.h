#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "server_logger.h"
#include "application.h"

namespace json_loader {
namespace json = boost::json;

std::string PrintMap(const model::Map& map);
std::string PrintMapList(const model::Game::Maps& map_list);
//std::string PrintErrorMsgJson(json::string_view code, json::string_view message);
std::string PrintPlayerList(const std::vector<app::PlayerPtr>& players);
std::string PrintPlayerState(const std::vector<app::PlayerPtr>& players);
//json::object MakePlayerListJson(const std::vector<app::PlayerPtr>& plist);
//json::object MakePlayerStateJson(const std::vector<app::PlayerPtr>& plist);

model::Game LoadGame(const std::filesystem::path& json_path);
} // namespace json_loader
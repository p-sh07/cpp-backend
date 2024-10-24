#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "application.h"

namespace json_loader {
namespace json = boost::json;

json::value MapToValue(const model::Map& map);

std::string PrintMap(const model::Map& map);
std::string PrintMapList(const model::Game::Maps& map_list);

const char ParseMove(const std::string& request_body);
model::TimeMs ParseTick(const std::string& request_body);

std::string PrintPlayerList(const std::vector<app::PlayerPtr>& players);
std::string PrintGameState(const app::PlayerPtr player, const std::shared_ptr<app::GameInterface>& game_app);

model::Game LoadGame(const std::filesystem::path& json_path);
} // namespace json_loader

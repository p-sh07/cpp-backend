//
// Created by ps on 8/22/24.
//

#include "application.h"
#include <iomanip>
#include <sstream>

//DEBUG
#include <iostream>

namespace {
app::Token GenerateToken() {
    thread_local static std::mt19937_64 gen1(std::random_device{}());
    thread_local static std::mt19937_64 gen2(std::random_device{}());

    using int64_num_t = std::mt19937_64::result_type;
    std::uniform_int_distribution<int64_num_t> dist(0, std::numeric_limits<int64_num_t>::max());

    std::stringstream ss;

    ss << std::hex << std::setfill('0') << std::setw(16) << dist(gen1);
    ss << std::hex << std::setfill('0') << std::setw(16) << dist(gen2);

    return app::Token{std::move(ss.str())};
}
}

namespace app {
Player::Player(size_t id, SessionPtr session, DogPtr dog)
    : id_(id)
    , session_(std::move(session))
    , dog_(std::move(dog)) {
}

PlayerManager::PlayerManager(const Game& game)
    : game_(game) {
}

Player& PlayerManager::Add(Player::Id player_id, Dog* dog, Session* session) {
    Player& player = players_.emplace_back(player_id, session, dog);

    //update indices
    auto token_result                 = token_to_player_.emplace(std::move(GenerateToken()), player_id);
    player_to_token_[player_id] = token_result.first;

    //post-increment next player id after final use here
    map_dog_id_to_player_[session->GetMapId()][dog->GetId()] = player_id;

    return player;
}

Player& PlayerManager::Add(model::Dog* dog, model::Session* session) {
    return Add(next_player_id_++, dog, session);
}

PlayerPtr PlayerManager::GetByToken(const Token& token) const {
    auto it = token_to_player_.find(token);

    //TODO: Avoid const_cast here?
    return it == token_to_player_.end() ? nullptr
    : const_cast<PlayerPtr>(&players_.at(it->second));
}

PlayerPtr PlayerManager::GetByMapDogId(const model::Map::Id& map_id, size_t dog_id) const {
    if(map_dog_id_to_player_.count(map_id) == 0 ||
        map_dog_id_to_player_.at(map_id).count(dog_id) == 0) {
        return nullptr;
    }
    //TODO: Avoid const_cast here?
    return const_cast<PlayerPtr>(&players_.at(map_dog_id_to_player_.at(map_id).at(dog_id)));
}

TokenPtr PlayerManager::GetToken(const Player& player) const {
    auto it = player_to_token_.find(player.GetId());
    if(it == player_to_token_.end()) {
        return nullptr;
    }
    auto& token = (it->second)->first;
    return &token;

}

const model::Session* PlayerManager::GetPlayerGameSession(PlayerPtr player) const {
    return player->GetSession();
}

const Session::LootItems& PlayerManager::GetSessionLootList(PlayerPtr player) const {
    return player->GetSession()->GetLootItems();
}

std::vector<app::PlayerPtr> PlayerManager::GetAllPlayersInSession(PlayerPtr player) const {
    std::vector<PlayerPtr> result;

    auto player_session = player->GetSession();
    const auto& map_id = player_session->GetMapId();

    for(const auto& [_, dog] : player_session->GetAllDogs()) {
        result.push_back(GetByMapDogId(map_id, dog.GetId()));
    }
    return result;
}

size_t TokenHasher::operator()(const Token& token) const {
    auto str_hasher = std::hash<std::string>();
    return str_hasher(*token);
}


std::vector<app::PlayerPtr> GameApp::GetPlayerList(PlayerPtr player) const {
    return players_.GetAllPlayersInSession(player);
}

const Session::LootItems& GameApp::GetLootList(PlayerPtr player) const {
    return players_.GetSessionLootList(player);
}

PlayerPtr GameApp::FindPlayerByToken(const Token& token) const {
    return players_.GetByToken(token);
}

JoinGameResult GameApp::JoinGame(std::string_view map_id_str, std::string_view player_dog_name) {
    const auto map_id = model::Map::Id(std::string(map_id_str));
    auto session = game_.FindSession(map_id);

    //no active session on map, make new session with separate strand
    if(!session) {
        session = game_.MakeSession(map_id, net::make_strand(io_));
    }

    auto dog = session->AddDog(std::move(std::string(player_dog_name)));
    auto player = players_.Add(dog, session);

    // <-Make response, send player token
    auto token = players_.GetToken(player);
    return {player.GetId(), token};
}

void GameApp::MovePlayer(PlayerPtr p, const char move_command) {
    //use default speed if map speed not set
    double speed_value = p->GetMap()->GetDogSpeed().has_value()
        ? p->GetMap()->GetDogSpeed().value() : game_.GetSettings().default_dog_speed;

    p->GetDog()->SetMove(static_cast<model::Dir>(move_command), speed_value);
}


const Game::Maps& GameApp::ListAllMaps() const {
    return game_.GetMaps();
}

const Map* GameApp::GetMap(std::string_view map_id) const {
    Map::Id id(std::move(std::string(map_id)));
    return game_.FindMap(id);
}

void GameApp::AdvanceGameTime(model::TimeMs delta_t) {
    game_.AdvanceTimeInSessions(delta_t);
}

}

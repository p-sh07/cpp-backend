//
// Created by ps on 8/22/24.
//

#include "application.h"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace app {
Player::Player(size_t id, SessionPtr session, DogPtr dog)
    : id_(id)
    , session_(std::move(session))
    , dog_(std::move(dog)) {
}

Players::Players(const GamePtr& game_ptr)
    : game_(game_ptr) {
}

Players::Players(GamePtr&& game)
    : game_(std::move(game)) {
}

Player& Players::Add(const model::Dog* dog, const model::Session* session) {
    Player& player = players_.emplace_back(next_player_id_, session, dog);

    //update indices
    auto token_result = token_to_player_.emplace(std::move(GenerateToken()), next_player_id_);
    player_to_token_[next_player_id_] = token_result.first;

    //post-increment next player id after final use here
    map_dog_id_to_player_[session->GetMapId()][dog->GetId()] = next_player_id_++;

    return player;
}

PlayerPtr Players::GetByToken(const Token& token) const {
    auto it = token_to_player_.find(token);

    //TODO: Avoid const_cast here?
    return it == token_to_player_.end() ? nullptr
    : const_cast<PlayerPtr>(&players_.at(it->second));
}

PlayerPtr Players::GetByMapDogId(const model::Map::Id& map_id, size_t dog_id) const {
    if(map_dog_id_to_player_.count(map_id) == 0 ||
        map_dog_id_to_player_.at(map_id).count(dog_id) == 0) {
        return nullptr;
    }
    //TODO: Avoid const_cast here?
    return const_cast<PlayerPtr>(&players_.at(map_dog_id_to_player_.at(map_id).at(dog_id)));
}

TokenPtr Players::GetToken(const Player& player) const {
    auto it = player_to_token_.find(player.GetId());
    if(it == player_to_token_.end()) {
        return nullptr;
    }
    auto& token = (it->second)->first;
    return &token;

}

Token Players::GenerateToken() const {
    std::uniform_int_distribution<std::mt19937_64::result_type> dist(0, std::numeric_limits<std::mt19937_64::result_type>::max());
    std::random_device random_device;
    std::mt19937_64 gen1(dist(random_device));
    std::mt19937_64 gen2(dist(random_device));

    std::stringstream ss;

    ss << std::hex << std::setfill('0') << std::setw(16) << dist(gen1);
    ss << std::hex << std::setfill('0') << std::setw(16)<< dist(gen2);

    return Token{std::move(ss.str())};
}

/*
    std::string random_string(std::string::size_type length)
    {
        static auto& chrs = "0123456789"
                            "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        thread_local static std::mt19937 rg{std::random_device{}()};
        thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

        std::string s;

        s.reserve(length);

        while(length--)
            s += chrs[pick(rg)];

        return s;
    }
*/

std::vector<PlayerPtr> Players::GetSessionPlayerList(const Player& player) {
    std::vector<PlayerPtr> result;

    auto player_session = player.GetSession();
    auto& map_id = player_session->GetMapId();

    for(const auto& dog : player_session->GetAllDogs()) {
        result.push_back(GetByMapDogId(map_id, dog.GetId()));
    }
    return result;
}

size_t TokenHasher::operator()(const Token& token) const {
    auto str_hasher = std::hash<std::string>();
    return str_hasher(*token);
}
std::vector<PlayerPtr> GameInterface::GetPlayerList(PlayerPtr player) {
    return players_.GetSessionPlayerList(*player);
}
PlayerPtr GameInterface::FindPlayerByToken(const Token& token) const {
    return players_.GetByToken(token);
}
std::pair<PlayerId, const Token*> GameInterface::JoinGame(std::string_view map_id_str, std::string_view player_dog_name) {
    auto session = game_->JoinSession(model::Map::Id(std::string(map_id_str)));
    auto dog = session->AddDog(std::move(std::string(player_dog_name)));
    auto player = players_.Add(dog, session);

    // <-Make response, send player token
    auto token = players_.GetToken(player);
    return {player.GetId(), token};
}
const Game::Maps& GameInterface::ListAllMaps() const {
    return game_->GetMaps();
}
const Map* GameInterface::GetMap(std::string_view map_id) const {
    Map::Id id(std::move(std::string(map_id)));
    return game_->FindMap(id);
}
GameInterface::GameInterface(const GamePtr& game_ptr)
    : game_(game_ptr)
    , players_(game_){
}
}
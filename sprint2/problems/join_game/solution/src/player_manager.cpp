//
// Created by ps on 8/22/24.
//

#include "player_manager.h"
#include <iomanip>
#include <sstream>

namespace app {
Player::Player(size_t id, SessionPtr session, DogPtr dog)
    : id_(id)
    , session_(std::move(session))
    , dog_(std::move(dog)) {
}

Players::Players(GamePtr game)
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

PlayerPtr Players::GetByToken(const Token& token) {
    auto it = token_to_player_.find(token);
    return it == token_to_player_.end() ? nullptr : &players_.at(it->second);
}

PlayerPtr Players::GetByMapDogId(const model::Map::Id& map_id, size_t dog_id) {
    if(map_dog_id_to_player_.count(map_id) == 0 ||
        map_dog_id_to_player_.at(map_id).count(dog_id) == 0) {
        return nullptr;
    }
    return &players_.at(map_dog_id_to_player_.at(map_id).at(dog_id));
}

TokenPtr Players::GetToken(const Player& player) {
    auto it = player_to_token_.find(player.GetId());
    if(it == player_to_token_.end()) {
        return nullptr;
    }
    auto& token = (it->second)->first;
    return &token;
}

Token Players::GenerateToken() const {
    //std::uniform_int_distribution<std::mt19937_64::result_type> dist(0, std::numeric_limits<std::mt19937_64::result_type>::max());
    auto t1 = generator1_;
    auto t2 = generator2_;

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << t1() << t2();
    return Token{std::move(ss.str())};
}

std::vector<PlayerPtr> Players::GetSessionPlayerList(const Player& player) {
    std::vector<PlayerPtr> result;

    auto player_session = player.GetSession();
    auto& map_id = player_session->GetMapId();

    for(const auto& dog : player_session->GetAllDogs()) {
        result.push_back(GetByMapDogId(map_id, dog.GetId()));
    }
    return result;
}

}
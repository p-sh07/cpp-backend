//
// Created by ps on 8/22/24.
//

#include "player_manager.h"

namespace app {
Player::Player(size_t id, SessionPtr session, DogPtr dog)
    : id_(id)
    , session_(std::move(session))
    , dog_(std::move(dog)) {
}

Players::Players(GamePtr game)
    : game_(std::move(game)) {
}

Player& Players::Add(model::Dog& dog, model::Session& session) {
    auto player_id = players_.size();
    Player& player = players_.emplace_back(player_id, std::shared_ptr<model::Session>(&session), std::shared_ptr<model::Dog>(&dog));

    //update indices
    auto token_id = tokens_.size();
    auto& token = tokens_.emplace_back(std::move(GenerateToken()));

    token_to_player_[&token] = player_id;
    player_to_token_[&player] = token_id;

    map_dog_id_to_player_[session.GetMapId()][dog.GetId()] = player_id;

    return player;
}

PlayerPtr Players::Get(const Token& token) {
    auto it = token_to_player_.find(&token);
    return it == token_to_player_.end() ? nullptr : &players_.at(it->second);
}

PlayerPtr Players::Get(const model::Map::Id& map_id, size_t dog_id) {
    if(map_dog_id_to_player_.count(map_id) == 0 ||
        map_dog_id_to_player_.at(map_id).count(dog_id) == 0) {
        return nullptr;
    }
    return &players_.at(map_dog_id_to_player_.at(map_id).at(dog_id));
}

TokenPtr Players::GetToken(const Player& player) {
    auto it = player_to_token_.find(const_cast<Player*>(&player));
    return it == player_to_token_.end() ? nullptr : &tokens_.at(it->second);
}

Token Players::GenerateToken() const {
    //std::uniform_int_distribution<std::mt19937_64::result_type> dist(0, std::numeric_limits<std::mt19937_64::result_type>::max());
    auto t1 = generator1_;
    auto t2 = generator2_;

    return Token{std::format("{:x}", t1()) + std::format("{:x}", t2())};
}

}
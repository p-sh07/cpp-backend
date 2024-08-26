//
// Created by Pavel on 22.08.24.
//
#pragma once
#include <list>
#include <memory>
#include <random>
#include <deque>
#include <unordered_map>
#include <format>

#include "tagged.h"
#include "model.h"

namespace detail {
struct TokenTag {};
}  // namespace detail


namespace app {
using SessionPtr = std::shared_ptr<model::Session>;
using DogPtr = std::shared_ptr<model::Dog>;

class Player {
 public:
    Player(size_t id, SessionPtr session, DogPtr dog);

    inline size_t GetId() const {
        return id_;
    }

 private:
    const size_t id_ = 0;
    SessionPtr session_;
    DogPtr dog_;
};

using Token = util::Tagged<std::string, detail::TokenTag>;

struct TokenHasher {
    size_t operator()(const Token& token) const {
        auto str_hasher = std::hash<std::string>();
        return str_hasher(*token);
    }
};

using GamePtr = std::shared_ptr<model::Game>;
using PlayerPtr = Player*;
using TokenPtr = const Token*;

class Players {
 public:
    explicit Players(GamePtr game);

    Player& Add(model::Dog& dog, model::Session& session);

    PlayerPtr Get(const Token& token);
    PlayerPtr Get(const model::Map::Id& map_id, size_t dog_id);
    TokenPtr GetToken(const Player& player);

 private:
    using PlayerId = size_t;
    using TokenId = size_t;
    using DogId = size_t;

    //TODO: use shared_ptr? Use iterator for easier delete?
    GamePtr game_;
    std::deque<Player> players_;
    std::deque<Token> tokens_;

    //Indices for search
    std::unordered_map<TokenPtr, PlayerId> token_to_player_;
    std::unordered_map<PlayerPtr, TokenId> player_to_token_;

    using MapDogIdToPlayer = std::unordered_map<model::Map::Id, std::unordered_map<DogId, PlayerId>, model::MapIdHasher>;
    MapDogIdToPlayer map_dog_id_to_player_;

    Token GenerateToken() const;

    //Token Generator
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    // Чтобы сгенерировать токен, получите из generator1_ и generator2_
    // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
    // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
    // чтобы сделать их подбор ещё более затруднительным
};

}
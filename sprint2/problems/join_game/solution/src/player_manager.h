//
// Created by Pavel on 22.08.24.
//
#pragma once
#include <list>
#include <memory>
#include <random>
#include <unordered_map>

#include "tagged.h"
#include "model.h"

namespace detail {
    struct TokenTag {};
}  // namespace detail


namespace app
{

    class Player {
        public:
        Player();
        ~Player();

    private:

    };

    using Token = util::Tagged<std::string, detail::TokenTag>;
    using PlayerPtr = Player*;
    using TokenPtr = const Token*;

    class Players {
    public:
        Players(std::shared_ptr<model::Game> game);

        Player& Add(const model::Dog& dog, const model::GameSession& session);

        PlayerPtr Get(const Token& token);
        PlayerPtr Get(size_t dog_id, size_t map_id);
        TokenPtr GetToken(const Player& player);

    private:
        std::shared_ptr<model::Game> game_;

        //List for quick erase from any place
        std::list<Player> players_;
        std::list<Token> tokens_;

        std::unordered_map<PlayerPtr, TokenPtr> token_map_;
        std::unordered_map<TokenPtr, PlayerPtr> player_map_;

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
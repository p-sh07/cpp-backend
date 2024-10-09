//
// Created by Pavel on 22.08.24.
//
#pragma once
#include <deque>
#include <filesystem>
#include <memory>
#include <random>
#include <unordered_map>

#include "app_util.h"
#include "model.h"

namespace detail {
struct TokenTag {};
}  // namespace detail

namespace app {
//static constexpr char MOVE_CMD_STOP = '!';
static constexpr char MOVE_CMD_ERR = '%';

using model::Game;
using model::Map;
using model::Dog;
using model::Session;

namespace fs = std::filesystem;

using SessionPtr = Session*;
using DogPtr = Dog*;

class Player {
 public:
    Player(size_t id, SessionPtr session, DogPtr dog);

    size_t GetId() const { return id_; }
    Dog* GetDog() { return dog_; }
    Session* GetSession() { return session_; }

    const Dog* GetDog() const { return dog_; }
    const Session* GetSession() const { return session_; }
    const Map* GetMap() const { return GetSession()->GetMap(); }

 private:
    const size_t id_ = 0;
    SessionPtr session_;
    DogPtr dog_;
};

using Token = util::Tagged<std::string, detail::TokenTag>;

struct TokenHasher {
    size_t operator()(const Token& token) const;
};

using GamePtr = std::shared_ptr<Game>;
using PlayerPtr = Player*;
using TokenPtr = const Token*;

using PlayerId = size_t;
using DogId = size_t;

class Players {
 public:
    explicit Players(const GamePtr& game);
    explicit Players(GamePtr&& game);

    Player& Add(Dog* dog, Session* session);

    PlayerPtr GetByToken(const Token& token) const;
    PlayerPtr GetByMapDogId(const  Map::Id& map_id, size_t dog_id) const;
    TokenPtr GetToken(const Player& player) const;

    std::vector<PlayerPtr> GetSessionPlayerList(const Player& player) const;
    const std::deque<model::Loot>& GetSessionLootList(const Player& player) const;

 private:
    GamePtr game_;
    std::deque<Player> players_;
    PlayerId next_player_id_ = 0;

    using TokenIt = std::unordered_map<Token, PlayerId>::iterator;

    //Indices for search
    std::unordered_map<Token, PlayerId, TokenHasher> token_to_player_;
    std::unordered_map<PlayerId, TokenIt> player_to_token_;

    using MapDogIdToPlayer = std::unordered_map< Map::Id, std::unordered_map<DogId, PlayerId>,  util::TaggedHasher<Map::Id>>;
    MapDogIdToPlayer map_dog_id_to_player_;

    Token GenerateToken() const;
};

struct JoinGameResult {
    PlayerId player_id;
    const Token* token;
};

class GameInterface {
 public:
    //GameInterface(const fs::path& game_config);
    GameInterface(GamePtr& game_ptr);

    //use cases
    const Map* GetMap(std::string_view map_id) const;
    const Game::Maps& ListAllMaps() const;

    void MovePlayer(PlayerPtr player, const char move_command);
    void AdvanceGameTime(model::TimeMs delta_t);

    JoinGameResult JoinGame(std::string_view map_id_str, std::string_view player_dog_name);
    PlayerPtr FindPlayerByToken(const Token& token) const;
    std::vector<PlayerPtr> GetPlayerList(PlayerPtr player) const;
    const std::deque<model::Loot>& GetLootList(PlayerPtr player) const;

 private:
    GamePtr game_;
    Players players_;

};

}

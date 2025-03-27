//
// Created by Pavel on 22.08.24.
//
#pragma once
#include <deque>
#include <filesystem>
#include <random>
#include <unordered_map>

//To enable strand for session
#include <boost/asio/io_context.hpp>

#include "util.h"
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
namespace net = boost::asio;

using SessionPtr = Session*;
using DogPtr = Dog*;

class Player {
 public:
    using Id = size_t;
    Player(size_t id, SessionPtr session, DogPtr dog);

    size_t GetId() const { return id_; }
    Dog* GetDog() { return dog_; }
    Session* GetSession() { return session_; }

    const Dog* GetDog() const { return dog_; }
    const Session* GetSession() const { return session_; }
    const Map* GetMap() const { return GetSession()->GetMap(); }

    size_t GetDogId() const { return dog_->GetId(); }
    model::Point2D GetPos() const { return dog_->GetPos(); }
    model::Speed GetSpeed() const { return dog_->GetSpeed(); }
    model::Dir GetDir() const { return dog_->GetDir(); }
    model::Dog::Bag GetBagItems() const { return dog_->GetBagItems(); }
    size_t GetScore() const { return dog_->GetScore(); }

 private:
    const Id id_ = 0u;
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

//Class for handling players and game sessions
//TODO:
//Game instance is kept inside the GameApp
// -> player manager is used to manage players in Game
//  -> init GameInterface with the GameSettings and path to config
class PlayerManager {
 public:
    explicit PlayerManager(const Game& game);
    //explicit PlayerManager(GamePtr&& game);

    Player& Add(Player::Id id, Dog* dog, Session* session);
    Player& Add(Dog* dog, Session* session);

    PlayerPtr GetByToken(const Token& token) const;
    PlayerPtr GetByMapDogId(const  Map::Id& map_id, size_t dog_id) const;
    TokenPtr GetToken(const Player& player) const;

    const Session* GetPlayerGameSession(PlayerPtr player) const;

    std::vector<app::PlayerPtr> GetAllPlayersInSession(PlayerPtr player) const;
    const Session::LootItems& GetSessionLootList(PlayerPtr player) const;

 private:
    const Game& game_;
    std::deque<Player> players_;
    Player::Id next_player_id_ = 0;

    using TokenIt = std::unordered_map<Token, Player::Id>::iterator;

    //Indices for search
    std::unordered_map<Token, Player::Id, TokenHasher> token_to_player_;
    std::unordered_map<Player::Id, TokenIt> player_to_token_;

    using MapDogIdToPlayer = std::unordered_map< Map::Id, std::unordered_map<Dog::Id, Player::Id>,  util::TaggedHasher<Map::Id>>;
    MapDogIdToPlayer map_dog_id_to_player_;
};

struct JoinGameResult {
    Player::Id player_id;
    const Token* token;
};

class GameApp {
 public:
    explicit GameApp(Game game, net::io_context& ioc)
        : io_(ioc)
        , game_(std::move(game))
        , players_(game_) {
    }

    //use cases
    const Map* GetMap(std::string_view map_id) const;
    const Game::Maps& ListAllMaps() const;

    void SetRandomDogSpawnPoint(bool enable) {
        game_.SetRandomDogSpawn(enable);
    }

    void MovePlayer(PlayerPtr player, const char move_command);
    void AdvanceGameTime(model::TimeMs delta_t);

    JoinGameResult JoinGame(std::string_view map_id_str, std::string_view player_dog_name);
    PlayerPtr FindPlayerByToken(const Token& token) const;

    //Returns the player's game session
    // const model::Session* GetSession(PlayerPtr player) const;

    //Returns vector of all players in same session as player
    std::vector<app::PlayerPtr> GetPlayerList(PlayerPtr player) const;
    const Session::LootItems& GetLootList(PlayerPtr player) const;

 private:
    net::io_context& io_;
    Game game_;
    PlayerManager players_;

};

}

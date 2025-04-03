//
// Created by Pavel on 22.08.24.
//
#pragma once
#include <boost/asio/io_context.hpp>
#include <deque>
#include <filesystem>
#include <ranges>
#include <unordered_map>

#include "app_util.h"
#include "database.h"
#include "model.h"
#include "loot_generator.h"

namespace detail {
struct TokenTag {};
}  // namespace detail

namespace app {
using namespace std::literals;
namespace fs = std::filesystem;
namespace net = boost::asio;

using model::Map;
using model::Dog;
using model::Game;
using model::GameObject;
using model::LootItem;

using model::MapPtr;
using model::DogPtr;
using model::ConstDogPtr;
using model::LootItemPtr;
using model::ItemsReturnPointPtr;
using model::CollisionObjectPtr;
using model::ItemsReturnPoint;

using Token = util::Tagged<std::string, detail::TokenTag>;

struct TokenHasher {
    size_t operator()(const Token& token) const;
};


//=================================================
//=================== Session =====================
class Session {
 public:
    using Id = size_t;
    using Dogs = std::unordered_map<Dog::Id, Dog>;
    using LootItems = std::deque<LootItemPtr>;
    using Offices = std::deque<ItemsReturnPointPtr>;

    using Gatherers = std::deque<DogPtr>;
    using CollisionObjects = std::deque<CollisionObjectPtr>;

    //TODO: make a sep. strand for session
    Session(Id id, MapPtr map, gamedata::Settings settings/*, net::io_context& io*/);

    size_t GetId() const;
    model::ConstMapPtr GetMap() const;
    model::TimeMs GetTime() const;

    const Map::Id& GetMapId() const;
    size_t GetDogCount() const;
    size_t GetLootCount() const;
    double GetDogSpeedVal() const;

    const Dogs& GetDogs() const;

    DogPtr GetDog(Dog::Id id);
    ConstDogPtr GetDog(Dog::Id id) const;

    const LootItems& GetLootItems() const;

    //At construction there are 0 dogs. Session is always on 1 map
    //When a player is added, he gets a new dog to control
    DogPtr AddDog(Dog dog);
    DogPtr AddDog(Dog::Id id, std::string name);

    void AddLootItem(LootItem::Id id, LootItem::Type type, model::Point2D pos);
    void AddRandomLootItems(size_t num_items);

    void RemoveDog(Dog::Id dog_id);
    void RemoveLootItem(GameObject::Id loot_item_id);

    std::vector<Dog::Id> AdvanceTime(model::TimeMs delta_t);

    // void EraseDog(Dog::Id dog_id);
    // void ClearRetiredDogs();
    void EraseRetiredDogs(const std::vector<Dog::Id>& retired_dog_ids);

private:
    //net::strand<net::io_context::executor_type> strand_;
    const Id id_;
    model::TimeMs session_time_ {0u};
    // size_t deleted_objects_counter_ {0u};
    // GameObject::Id next_dog_id_ {0u};
    GameObject::Id next_object_id_ {0u};

    Dogs dogs_;
    LootItems loot_items_;
    Offices offices_;

    Gatherers gatherers_;
    CollisionObjects objects_;

    MapPtr map_;
    gamedata::Settings settings_;
    loot_gen::LootGenerator loot_generator_;

    void AddOffices(const Map::Offices& offices);

    void MoveDog(Dog& dog, model::TimeMs delta_t) const;
    void MoveAllDogs(model::TimeMs delta_t);

    void GenerateLoot(model::TimeMs delta_t);
    void ProcessCollisions() const;
    std::vector<Dog::Id> GetRetiredDogs();
};

using SessionPtr = Session*;
using ConstSessionPtr = const Session*;


//=================================================
//=================== Player ======================
class Player {
 public:
    using Id = size_t;
    Player(Id id, SessionPtr session, DogPtr dog);

    size_t GetId() const { return id_; }
    DogPtr GetDog() { return dog_; }
    SessionPtr GetSession() { return session_; }

    model::ConstDogPtr GetDog() const { return dog_; }
    model::ConstMapPtr GetMap() const { return GetSession()->GetMap(); }
    SessionPtr GetSession() const { return session_; }

    size_t GetScore() const { return dog_->GetScore(); }
    void SetDirection(model::Direction dir) const;

 private:
    const Id id_;
    SessionPtr session_;
    DogPtr dog_;
};

using GamePtr = std::shared_ptr<Game>;
using PlayerPtr = Player*;
using ConstPlayerPtr = const Player*;
using TokenPtr = const Token*;


//=================================================
//========= Player & Session Manager ==============
class PlayerSessionManager {
public:
    using Players = std::unordered_map<Player::Id, Player>;
    using Sessions = std::unordered_map<Session::Id, Session>;
    using TokenToPlayer = std::unordered_map<Token, Player::Id, TokenHasher>;
    using MapToSession = std::unordered_map<Map::Id, Session::Id, util::TaggedHasher<Map::Id>>;

    explicit PlayerSessionManager(const GamePtr& game);
    explicit PlayerSessionManager(GamePtr&& game);

    PlayerSessionManager(const GamePtr& game, Sessions sessions);

    PlayerPtr CreatePlayer(const Map::Id& map, std::string dog_name);
    PlayerPtr AddPlayer(Player::Id id, DogPtr dog, SessionPtr session, Token token);
    PlayerPtr RestorePlayer(Player::Id id, Dog::Id dog_id, Session::Id session_id, Token token);

    SessionPtr JoinOrCreateSession(Session::Id session_id, const Map::Id& map_id);
    void RemoveSession(Session::Id session_id);

    TokenPtr GetToken(Player::Id player_id) const;
    TokenPtr GetToken(ConstPlayerPtr player) const;

    const Players& GetAllPlayers() const;
    const Sessions& GetAllSessions() const;

    ConstPlayerPtr GetPlayerByToken(const Token& token) const;
    ConstPlayerPtr GetPlayerByMapDogId(const Map::Id& map_id, size_t dog_id) const;

    static ConstSessionPtr GetPlayerGameSession(ConstPlayerPtr player);
    std::vector<ConstPlayerPtr> GetAllPlayersInSession(ConstPlayerPtr player) const;
    static const Session::LootItems& GetSessionLootList(ConstPlayerPtr player);

    gamedata::PlayerStats RetirePlayer(const Map::Id map_id, Dog::Id dog_id);
    std::vector<gamedata::PlayerStats> AdvanceTime(model::TimeMs delta_t);

private:
    GamePtr game_;
    Players players_;
    Sessions sessions_;

    Dog::Id next_dog_id_ {0u};
    Player::Id next_player_id_ {0u};
    Session::Id next_session_id_ {0u};

    //Indices for search
    using TokenIt = std::unordered_map<Token, Player::Id>::iterator;
    TokenToPlayer token_to_player_;
    std::unordered_map<Player::Id, TokenIt> player_to_token_;

    MapToSession map_to_session_index_;

    using MapDogIdToPlayer = std::unordered_map<Map::Id, std::unordered_map<Dog::Id, Player::Id>,  util::TaggedHasher<Map::Id>>;
    MapDogIdToPlayer map_dog_id_to_player_index_;

};


//=================================================
//=================== App Listener ================
class ApplicationListener {
public:
    virtual void OnTick(model::TimeMs delta_t, const app::PlayerSessionManager& psm) = 0;
    virtual PlayerSessionManager Restore(GamePtr game) const = 0;
protected:
    virtual ~ApplicationListener() = default;
};

using AppListenerPtr = std::shared_ptr<ApplicationListener>;


//=================================================
//================= GameInterface =================
struct JoinGameResult {
    Player::Id player_id;
    const Token* token;
};

class GameInterface {
 public:
    //GameInterface(const fs::path& game_config);
    GameInterface(net::io_context& io, GamePtr game_ptr, AppListenerPtr app_listener_ptr, database::PlayerStats& player_db);

    //use cases
    model::ConstMapPtr GetMap(std::string_view map_id) const;
    const Game::Maps& ListAllMaps() const;

    bool MoveCommandValid(char move_command) const;
    void SetPlayerMovement(ConstPlayerPtr player, char move_command);
    void AdvanceGameTime(model::TimeMs delta_t);

    const PlayerSessionManager& GetPlayerManager() const;
    void RestorePlayerManagerState(PlayerSessionManager psm);

    JoinGameResult JoinGame(std::string map_id_str, std::string player_dog_name);
    ConstPlayerPtr FindPlayerByToken(const Token& token) const;

    //Returns the player's game session
    ConstSessionPtr GetSession(ConstPlayerPtr player) const;

    //Returns vector of all players in same session as player
    std::vector<ConstPlayerPtr> GetPlayerList(ConstPlayerPtr player) const;
    const Session::LootItems& GetLootList(ConstPlayerPtr player) const;
    std::vector<gamedata::PlayerStats> GetPlayerStats(std::optional<size_t> start, std::optional<size_t> max_players);

private:
    net::io_context& io_;
    AppListenerPtr app_listener_ = nullptr;
    GamePtr game_;
    PlayerSessionManager player_manager_;
    database::PlayerStats& player_stat_db_;

    //TODO: use from GameSettings
    static constexpr auto valid_move_chars_ = "UDLR"sv;

};
}

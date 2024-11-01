//
// Created by Pavel on 22.08.24.
//
#pragma once
#include <deque>
#include <filesystem>
#include <random>
#include <unordered_map>

#include "app_util.h"
#include "model.h"
#include "loot_generator.h"

namespace detail {
struct TokenTag {};
}  // namespace detail

namespace app {
using namespace std::literals;
namespace fs = std::filesystem;

using model::Map;
using model::Dog;
using model::Game;
using model::GameObject;
using model::LootItem;

using Token = util::Tagged<std::string, detail::TokenTag>;

struct TokenHasher {
    size_t operator()(const Token& token) const;
};


//=================================================
//=================== Session =====================
class Session {
    using ObjectMap = std::unordered_map<GameObject::Id, size_t>;

 public:
    using Id = size_t;
    using Dogs = std::deque<Dog>;
    using LootItems = std::unordered_map<LootItem::Id, LootItem>;
    using Offices = std::deque<model::ItemsReturnPoint>;

    Session(Id id, model::MapPtr map, model::GameSettings settings);

    size_t GetId() const;
    model::ConstMapPtr GetMap() const;
    const Map::Id& GetMapId() const;
    size_t GetDogCount() const;
    size_t GetLootCount() const;
    double GetDogSpeedVal() const;

    const Dogs& GetDogs() const;
    const LootItems& GetLootItems() const;

    //At construction there are 0 dogs. Session is always on 1 map
    //When a player is added, he gets a new dog to control
    model::DogPtr AddDog(Dog::Id, Dog::Tag name);
    void AddLootItem(LootItem::Id id, LootItem::Type type, model::Point2D pos);
    void AddRandomLootItems(size_t num_items);

    void RemoveDog(model::DogPtr dog);
    void RemoveLootItem(model::GameObject::Id loot_item_id);

    void AdvanceTime(model::TimeMs delta_t);

private:
    const Id id_;
    model::TimeMs time_ {0u};
    size_t deleted_objects_counter_ {0u};
    model::GameObject::Id next_dog_id_ {0u};
    model::GameObject::Id next_object_id_ {0u};

    Dogs dogs_;
    LootItems loot_items_;
    Offices offices_;

    ObjectMap dogs_index_;

    //After delete, obj in deq is set to nullptr. This updates map and removes nulls
    template<typename ObjContainer, typename ObjIndex>
    void UpdObjIndex(ObjContainer& linear_container, ObjIndex& map);

    model::MapPtr map_;
    model::GameSettings settings_;
    loot_gen::LootGenerator loot_generator_;
    model::ItemEventHandler<LootItems, Offices, Dogs> collision_provider_;

    void MoveDog(Dog& dog, model::TimeMs delta_t);
    void MoveAllDogs(model::TimeMs delta_t);

    void GenerateLoot(model::TimeMs delta_t);
    void ProcessCollisions() const;

    void HandleCollision(const model::LootItemPtr& loot, const model::DogPtr& dog) const;
    void HandleCollision(const model::ItemsReturnPointPtr& office, const model::DogPtr& dog) const;
};

template<typename ObjContainer, typename ObjIndex>
void Session::UpdObjIndex(ObjContainer& linear_container, ObjIndex& map) {
    size_t idx = 0;
    map.clear();
    for(auto it = linear_container.begin(); it != linear_container.end(); ) {
        auto obj_ptr = *it;
        if(!obj_ptr) {
            //nullptr -> erase
            it = linear_container.erase(it);
        } else {
            //obj present -> add to index
            map.emplace(obj_ptr->GetId(), idx);
            ++idx;
            ++it;
        }
    }
}

using SessionPtr = std::shared_ptr<Session>;
using ConstSessionPtr = std::shared_ptr<const Session>;


//=================================================
//=================== Player ======================
class Player {
 public:
    using Id = size_t;
    Player(Id id, SessionPtr session, model::DogPtr dog);

    size_t GetId() const { return id_; }
    model::DogPtr GetDog() { return dog_; }
    SessionPtr GetSession() { return session_; }

    model::ConstDogPtr GetDog() const { return dog_; }
    model::ConstMapPtr GetMap() const { return GetSession()->GetMap(); }
    SessionPtr GetSession() const { return session_; }

    size_t GetScore() const { return dog_->GetScore(); }
    void SetDirection(model::Direction dir) const;

 private:
    const Id id_;
    SessionPtr session_;
    model::DogPtr dog_;
};

using GamePtr = std::shared_ptr<Game>;
using PlayerPtr = Player*;
using ConstPlayerPtr = const Player*;
using TokenPtr = const Token*;


//=================================================
//========= Player & Session Manager ==============
class PlayerSessionManager {
 public:
    using Players = std::deque<Player>;
    using Sessions = std::deque<Session>;
    using TokenToPlayer = std::unordered_map<Token, Player::Id, TokenHasher>;
    using MapToSession = std::unordered_map<Map::Id, size_t, util::TaggedHasher<Map::Id>>;

    explicit PlayerSessionManager(const GamePtr& game);
    explicit PlayerSessionManager(GamePtr&& game);

    PlayerPtr CreatePlayer(Map::Id map, Dog::Tag dog_tag);
    PlayerPtr AddPlayer(Player::Id id, model::DogPtr dog, SessionPtr session, Token token);

    SessionPtr JoinOrCreateSession(Session::Id session_id, const Map::Id& map_id);
    TokenPtr GetToken(const PlayerPtr& player) const;

    const TokenToPlayer& GetAllTokens() const;
    const Players& GetAllPlayers() const;
    const Sessions& GetAllSessions() const;

    ConstPlayerPtr GetPlayerByToken(const Token& token) const;
    ConstPlayerPtr GetPlayerByMapDogId(const Map::Id& map_id, size_t dog_id) const;

    static ConstSessionPtr GetPlayerGameSession(ConstPlayerPtr& player);
    std::vector<ConstPlayerPtr> GetAllPlayersInSession(ConstPlayerPtr& player) const;
    static const Session::LootItems& GetSessionLootList(ConstPlayerPtr& player);

    void AdvanceTime(model::TimeMs delta_t);

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

    using MapDogIdToPlayer = std::unordered_map< Map::Id, std::unordered_map<Dog::Id, Player::Id>,  util::TaggedHasher<Map::Id>>;
    MapDogIdToPlayer map_dog_id_to_player_index_;
};


//=================================================
//================= GameInterface =================
struct JoinGameResult {
    Player::Id player_id;
    const Token* token;
};

class GameInterface {
 public:
    //GameInterface(const fs::path& game_config);
    explicit GameInterface(const GamePtr& game_ptr);

    //use cases
    model::ConstMapPtr GetMap(std::string_view map_id) const;
    const Game::Maps& ListAllMaps() const;

    static bool MoveCommandValid(const char move_command);
    static void SetPlayerMovement(const PlayerPtr& player, const char move_command);
    void AdvanceGameTime(model::TimeMs delta_t);

    JoinGameResult JoinGame(std::string_view map_id_str, std::string_view player_dog_name);
    ConstPlayerPtr FindPlayerByToken(const Token& token) const;

    //Returns the player's game session
    ConstSessionPtr GetSession(ConstPlayerPtr& player) const;

    //Returns vector of all players in same session as player
    std::vector<ConstPlayerPtr> GetPlayerList(ConstPlayerPtr& player) const;
    const Session::LootItems& GetLootList(ConstPlayerPtr& player) const;

 private:
    GamePtr game_;
    PlayerSessionManager player_manager_;

    //TODO: move to GameSettings
    static constexpr auto valid_move_chars_ = "UDLR"sv;

};

}

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
    using Dogs = std::deque<model::DogPtr>;
    using LootItems = std::deque<model::LootItemPtr>;
    using Offices = std::deque<model::ItemsReturnPointPtr>;

    Session(size_t id, model::MapPtr map, model::GameSettings settings);

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
    model::DogPtr AddDog(std::string name);
    void AddLootItem(LootItem::Type type, model::Point2D pos);
    void AddRandomLootItems(size_t num_items);

    void RemoveDog(model::DogPtr dog);
    void RemoveLootItem(model::GameObject::Id loot_item_id);

    void AdvanceTime(model::TimeMs delta_t);

 private:
    const size_t id_;
    model::TimeMs time_ {0u};
    size_t deleted_objects_counter_ {0u};
    model::GameObject::Id next_dog_id_ {0u};
    model::GameObject::Id next_object_id_ {0u};

    Dogs dogs_;
    LootItems loot_items_;
    Offices offices_;

    ObjectMap dogs_index_;
    ObjectMap objects_index_;

    //After delete, obj in deq is set to nullptr. This updates map and removes nulls
    template<typename ObjContainer>
    void UpdObjIndex(ObjContainer& linear_container, ObjContainer& map);

    model::MapPtr map_;
    model::GameSettings settings_;
    loot_gen::LootGenerator loot_generator_;
    model::ItemEventHandler<LootItems, Offices, Dogs> collision_provider_;

    void MoveDog(const model::DogPtr& dog, model::TimeMs delta_t) const;
    void MoveAllDogs(model::TimeMs delta_t) const;

    void GenerateLoot(model::TimeMs delta_t);
    void ProcessCollisions() const;

    static void HandleCollision(const model::LootItemPtr& loot, const model::DogPtr& dog);
    static void HandleCollision(const model::ItemsReturnPointPtr& office, const model::DogPtr& dog) const;
};

template<typename ObjContainer>
void Session::UpdObjIndex(ObjContainer& linear_container, ObjContainer& map) {
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
    Player(size_t id, SessionPtr session, model::DogPtr dog);

    size_t GetId() const { return id_; }
    model::DogPtr GetDog() { return dog_; }
    SessionPtr GetSession() { return session_; }

    model::ConstDogPtr GetDog() const { return dog_; }
    model::ConstMapPtr GetMap() const { return GetSession()->GetMap(); }
    SessionPtr GetSession() const { return session_; }

    size_t GetScore() const { return dog_->GetScore(); }
    void SetDirection(model::Direction dir) const;

 private:
    const size_t id_;
    SessionPtr session_;
    model::DogPtr dog_;
};

using GamePtr = std::shared_ptr<Game>;
using PlayerPtr = std::shared_ptr<Player>;
using ConstPlayerPtr = std::shared_ptr<const Player>;
using TokenPtr = const Token*;

using PlayerId = size_t;
using DogId = size_t;
using SessionId = size_t;


//=================================================
//========= Player & Session Manager ==============
class PlayerManager {
    using Players = std::deque<PlayerPtr>;
 public:
    using MapSessions = std::unordered_set<SessionPtr>;
    using MapToSessions = std::unordered_map<Map::Id, MapSessions, util::TaggedHasher<Map::Id>>;

    explicit PlayerManager(const GamePtr& game);
    explicit PlayerManager(GamePtr&& game);

    PlayerPtr Add(model::DogPtr dog, SessionPtr session);

    PlayerPtr GetPlayerByToken(const Token& token) const;
    PlayerPtr GetPlayerByMapDogId(const Map::Id& map_id, size_t dog_id) const;
    TokenPtr GetToken(const PlayerPtr& player) const;

    const MapToSessions& GetAllSessionsByMap() const;

    SessionPtr JoinOrCreateSession(const Map::Id& map_id);
    static ConstSessionPtr GetPlayerGameSession(const PlayerPtr& player);
    std::vector<PlayerPtr> GetAllPlayersInSession(const PlayerPtr& player) const;
    static const Session::LootItems& GetSessionLootList(const PlayerPtr& player);

 private:
    GamePtr game_;
    PlayerId next_player_id_ {0u};
    SessionId next_session_id_ {0u};

    Players players_;

    //Indices for search
    using TokenIt = std::unordered_map<Token, PlayerId>::iterator;
    std::unordered_map<Token, PlayerId, TokenHasher> token_to_player_;
    std::unordered_map<PlayerId, TokenIt> player_to_token_;

    MapToSessions map_to_sessions_;

    using MapDogIdToPlayer = std::unordered_map< Map::Id, std::unordered_map<DogId, PlayerId>,  util::TaggedHasher<Map::Id>>;
    MapDogIdToPlayer map_dog_id_to_player_index_;

    using MapAndSessions = std::pair<model::MapPtr, PlayerManager::MapSessions*>;

    MapAndSessions GetMapSessions(const Map::Id& map_id);
    //TODO: multiple sessions, using only one session per map for now
    static SessionPtr JoinFreeSession(const MapSessions* map_sessions_ptr);
};


//=================================================
//================= GameInterface =================
struct JoinGameResult {
    PlayerId player_id;
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
    void AdvanceGameTime(model::TimeMs delta_t) const;

    JoinGameResult JoinGame(std::string_view map_id_str, std::string_view player_dog_name);
    PlayerPtr FindPlayerByToken(const Token& token) const;

    //Returns the player's game session
    ConstSessionPtr GetSession(const PlayerPtr& player) const;

    //Returns vector of all players in same session as player
    std::vector<PlayerPtr> GetPlayerList(const PlayerPtr& player) const;
    const Session::LootItems& GetLootList(const PlayerPtr& player) const;

 private:
    GamePtr game_;
    PlayerManager player_manager_;

    //TODO: move to GameSettings
    static constexpr auto valid_move_chars_ = "UDLR"sv;

};

}

//
// Created by ps on 8/22/24.
//

#include "application.h"
#include <iomanip>
#include <sstream>

//DEBUG
#include <iostream>

namespace {
app::Token GenerateToken() {
    thread_local static std::mt19937_64 gen1(std::random_device{}());
    thread_local static std::mt19937_64 gen2(std::random_device{}());

    using int64_num_t = std::mt19937_64::result_type;
    std::uniform_int_distribution<int64_num_t> dist(0, std::numeric_limits<int64_num_t>::max());

    std::stringstream ss;

    ss << std::hex << std::setfill('0') << std::setw(16) << dist(gen1);
    ss << std::hex << std::setfill('0') << std::setw(16) << dist(gen2);

    return app::Token{std::move(ss.str())};
}
}

namespace app {
size_t TokenHasher::operator()(const Token& token) const {
    auto str_hasher = std::hash<std::string>();
    return str_hasher(*token);
}

//=================================================
//=================== Session =====================
Session::Session(size_t id, model::MapPtr map, model::GameSettings settings)
    : id_(id)
      , map_(map)
      , settings_(std::move(settings))
      , loot_generator_(settings_.loot_gen_interval, settings_.loot_gen_prob)
      , collision_provider_(loot_items_, offices_, dogs_) {
    //Set map bag & speed settings if specified
    settings_.map_dog_speed = map_->GetDogSpeed();
    settings_.map_bag_capacity = map_->GetBagCapacity();

    for(const auto& office : map_->GetOffices()) {
        offices_.emplace_back(std::make_shared<model::ItemsReturnPoint>(
            next_object_id_++, office, settings_.office_width
        ));
    }
}

size_t Session::GetId() const {
    return id_;
}

model::ConstMapPtr Session::GetMap() const {
    return map_;
}

const Map::Id &Session::GetMapId() const {
    return map_->GetId();
}

size_t Session::GetDogCount() const {
    return dogs_.size();
}

size_t Session::GetLootCount() const {
    return loot_items_.size();
}

double Session::GetDogSpeedVal() const {
    return settings_.GetDogSpeed();
}

const Session::Dogs &Session::GetDogs() const {
    return dogs_;
}

const Session::LootItems &Session::GetLootItems() const {
    return loot_items_;
}

//---------------------------------------------------------
model::DogPtr Session::AddDog(Dog::Id id, Dog::Tag name) {
    auto starting_pos = settings_.randomised_dog_spawn
                            ? map_->GetRandomRoadPt()
                            : map_->GetFirstRoadPt();

    const size_t index = dogs_.size();
    auto& dog = dogs_.emplace_back(std::make_shared<Dog>(
        id, starting_pos, settings_.dog_width
        , Dog::Tag(std::move(name)), settings_.GetBagCap()
    ));

    try {
        dogs_index_.emplace(id, index);
    } catch (...) {
        dogs_.pop_back();
        throw std::runtime_error("Failed to add new dog to session");
    }
    //Creation successful
    return dog;
}

void Session::AddLootItem(LootItem::Id id, LootItem::Type type, model::Point2D pos) {
    loot_items_.emplace_back(std::make_shared<LootItem>(
        id, pos, settings_.loot_item_width, type
    ));
}

void Session::AddRandomLootItems(size_t num_items) {
    for (int i = 0; i < num_items; ++i) {
        AddLootItem(
            next_object_id_++,
            model::GenRandomNum(map_->GetLootTypesSize()),
            map_->GetRandomRoadPt()
        );
    }
}

void Session::RemoveDog(model::DogPtr dog) {
    //TODO:
}

void Session::RemoveLootItem(GameObject::Id loot_item_id) {
    //TODO:
}

void Session::AdvanceTime(model::TimeMs delta_t) {
    time_ += delta_t;

    MoveAllDogs(delta_t);
    ProcessCollisions();

    //Generate loot after, so that a loot item is not randomly picked up by dog
    GenerateLoot(delta_t);
}

//---------------------------------------------------------
void Session::MoveDog(const model::DogPtr& dog, model::TimeMs delta_t) {
    //Operators for time-distance calculations
    using model::operator+;
    using model::operator*;

    model::Point2D max_move_pt = dog->GetPos() + dog->GetSpeed() * delta_t;
    Map::MoveResult move_result = map_->ComputeRoadMove(dog->GetPos(), max_move_pt);
    if (move_result.road_edge_reached_) {
        dog->Stop();
    }
    dog->SetPos(move_result.dst);
}

void Session::MoveAllDogs(model::TimeMs delta_t) {
    for (auto& dog : dogs_) {
        MoveDog(dog, delta_t);
    }
}

void Session::GenerateLoot(model::TimeMs delta_t) {
#ifdef GATHER_DEBUG
    if(loot_items_.empty()) {
        AddLootItem(0, {0.0, 50.0});
    }
#else
    auto num_of_new_items = loot_generator_.Generate(delta_t, GetLootCount(), GetDogCount());
    AddRandomLootItems(num_of_new_items);
#endif
}

//---------------------------------------------------------
void Session::ProcessCollisions() const {
    auto collision_events = collision_provider_.FindCollisions();
    for (const auto& event : collision_events) {
        //TODO: refactor this later...
        // if (event.is_loot_collect) {
        //     HandleCollision(loot_items_.at(event.obj_id), dogs_.at(event.dog_id));
        // } else if (event.is_items_return) {
        //     HandleCollision(offices_.at(event.obj_id), dogs_.at(event.dog_id));
        // }

    }
}

void Session::HandleCollision(const model::LootItemPtr& loot, const model::DogPtr& dog) const {
    if (!loot->IsCollected()) {
        dog->TryCollectItem(loot);
    }
}

void Session::HandleCollision(const model::ItemsReturnPointPtr& office, const model::DogPtr& dog) const {
    for (const auto& item : dog->GetBag()) {
        dog->AddScore(map_->GetLootItemValue(item.type));
    }
    dog->ClearBag();
}


//=================================================
//=================== Player ======================
Player::Player(size_t id, SessionPtr session, model::DogPtr dog)
    : id_(id)
      , session_(std::move(session))
      , dog_(std::move(dog)) {
}

void Player::SetDirection(model::Direction dir) const {
    dog_->SetMovement(dir, session_->GetDogSpeedVal());
}

//=================================================
//=============PlayerManager ======================
PlayerSessionManager::PlayerSessionManager(const GamePtr& game)
    : game_(game) {
}

PlayerSessionManager::PlayerSessionManager(GamePtr&& game)
    : game_(std::move(game)) {
}

PlayerPtr PlayerSessionManager::CreatePlayer(Map::Id map, Dog::Tag dog_tag) {
    auto session = JoinOrCreateSession(next_session_id_++, map);
    auto dog = session->AddDog(next_dog_id_++, dog_tag);
    return AddPlayer(next_player_id_++, dog, session, GenerateToken())
}


PlayerPtr PlayerSessionManager::AddPlayer(Player::Id id, model::DogPtr dog, SessionPtr session, Token token) {
    //TODO: check for duplicate token? Catch error and pop back player?
    PlayerPtr player_ptr = &players_.emplace_back(id, session, dog);
    size_t index = players_.size();

    //update indices
    auto token_result = token_to_player_.emplace(std::move(token), index);
    player_to_token_[id] = token_result.first;
    map_dog_id_to_player_index_[session->GetMapId()][dog->GetId()] = index;

    //Success;
    return player_ptr;
}

ConstPlayerPtr PlayerSessionManager::GetPlayerByToken(const Token& token) const {
    if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
         return &players_.at(it->second);
    }
    return nullptr;
}

ConstPlayerPtr PlayerSessionManager::GetPlayerByMapDogId(const model::Map::Id& map_id, size_t dog_id) const {
    if (!map_dog_id_to_player_index_.contains(map_id)
        || !map_dog_id_to_player_index_.at(map_id).contains(dog_id)) {
        return nullptr;
    }
    return &players_.at(map_dog_id_to_player_index_.at(map_id).at(dog_id));
}

TokenPtr PlayerSessionManager::GetToken(const PlayerPtr& player) const {
    auto it = player_to_token_.find(player->GetId());
    if (it == player_to_token_.end()) {
        return nullptr;
    }
    auto& token = (it->second)->first;
    return &token;
}

const PlayerSessionManager::TokenToPlayer& PlayerSessionManager::GetAllTokens() const {
    return token_to_player_;
}

const PlayerSessionManager::Players& PlayerSessionManager::GetAllPlayers() const {
    return players_;
}

const PlayerSessionManager::Sessions& PlayerSessionManager::GetAllSessions() const {
    return sessions_;
}

SessionPtr PlayerSessionManager::JoinOrCreateSession(Session::Id session_id, const Map::Id& map_id) {
    //Check if a session exixts on map. GetMapSessions checks that mapid is valid
    const auto map_ptr = game_->FindMap(map_id);
    if(!map_ptr) {
        //DEBUG
        std::cerr << "Cannot join session, map not found";
        return nullptr;
    }
    //No sessions on map, create new session
    auto session_it = map_to_session_index_.find(map_id);
    if(session_it == map_to_session_index_.end()) {
        auto session_idx = sessions_.size();
        auto& session = sessions_.emplace_back(
            session_id, map_ptr, std::move(game_->GetSettings())
        );

        try {
            map_to_session_index_.emplace(map_id, session_idx);
            return std::make_shared<Session>(session);
        } catch (std::exception& ex) {
            //DEBUG
            std::cerr << "Failed to create new session on map: " << ex.what();
            throw std::runtime_error("Failed to make a new session on map");
        }
    }
    //A session exists and can be joined
    return std::make_shared<Session>(sessions_.at(session_it->second));
}

ConstSessionPtr PlayerSessionManager::GetPlayerGameSession(ConstPlayerPtr& player) {
    return player->GetSession();
}

std::vector<ConstPlayerPtr> PlayerSessionManager::GetAllPlayersInSession(ConstPlayerPtr& player) const {
    std::vector<ConstPlayerPtr> result;

    auto player_session = player->GetSession();
    const auto& map_id = player_session->GetMapId();

    for (const auto& dog : player_session->GetDogs()) {
        result.push_back(GetPlayerByMapDogId(map_id, dog->GetId())->);
    }
    return result;
}

const Session::LootItems &PlayerSessionManager::GetSessionLootList(ConstPlayerPtr& player) {
    return player->GetSession()->GetLootItems();
}

void PlayerSessionManager::AdvanceTime(model::TimeMs delta_t) {
    for(auto& session : sessions_) {
        session.AdvanceTime(delta_t);
    }
}


//=================================================
//============= GameInterface =====================
GameInterface::GameInterface(const GamePtr& game_ptr)
    : game_(game_ptr)
      , player_manager_(game_) {
}

ConstSessionPtr GameInterface::GetSession(ConstPlayerPtr& player) const {
    return player_manager_.GetPlayerGameSession(player);
}

std::vector<ConstPlayerPtr> GameInterface::GetPlayerList(ConstPlayerPtr& player) const {
    return player_manager_.GetAllPlayersInSession(player);
}

const Session::LootItems &GameInterface::GetLootList(ConstPlayerPtr& player) const {
    return player_manager_.GetSessionLootList(player);
}

ConstPlayerPtr GameInterface::FindPlayerByToken(const Token& token) const {
    return player_manager_.GetPlayerByToken(token);
}

JoinGameResult GameInterface::JoinGame(std::string_view map_id_str, std::string_view player_dog_name) {
    Map::Id map_id{std::move(std::string(map_id_str))};
    Dog::Tag dog_tag{std::move(std::string(player_dog_name))};

    auto player = player_manager_.CreatePlayer(map_id, dog_tag);

    // <-Make response, send player token
    auto token = player_manager_.GetToken(player);
    return {player->GetId(), token};
}

void GameInterface::SetPlayerMovement(const PlayerPtr& player, const char move_command) {
    //use game default speed if map speed not set
    auto dir = static_cast<model::Direction>(move_command);
    player->SetDirection(dir);
}


const Game::Maps &GameInterface::ListAllMaps() const {
    return game_->GetMaps();
}

model::ConstMapPtr GameInterface::GetMap(std::string_view map_id) const {
    return game_->FindMap(Map::Id(std::string(map_id)));
}

void GameInterface::AdvanceGameTime(model::TimeMs delta_t) {
    player_manager_.AdvanceTime(delta_t);
}

bool GameInterface::MoveCommandValid(const char move_command) {
    return !isblank(move_command)
           && valid_move_chars_.find(move_command) != valid_move_chars_.npos;
}
} //namespace app
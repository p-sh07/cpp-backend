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
        offices_.push_back(std::make_shared<model::ItemsReturnPoint>(
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
model::DogPtr Session::AddDog(std::string name) {
    auto starting_pos = settings_.randomise_dog_spawn
                            ? map_->GetRandomRoadPt()
                            : map_->GetFirstRoadPt();

    model::DogPtr dog = nullptr;
    const size_t index = dogs_.size();
    try {
        dog = std::make_shared<Dog>(next_dog_id_, starting_pos, settings_.dog_width
                                    , Dog::Tag(std::move(name)), settings_.GetBagCap());
        dogs_.push_back(dog);
        dogs_index_.emplace(next_dog_id_, index);
    } catch (...) {
        // Уничтожаем собаку если произошла ошибка
        dog.reset();
        dogs_.pop_back();
        throw std::runtime_error("Failed to add new dog to session");
    }
    //Creation successful
    ++next_dog_id_;
    return dog;
}

void Session::AddLootItem(LootItem::Type type, model::Point2D pos) {
    model::LootItemPtr loot = nullptr;
    const size_t index = loot_items_.size();
    try {
        loot = std::make_shared<LootItem>(next_object_id_, pos
                                          , settings_.loot_item_width, type);
        loot_items_.push_back(loot);
        objects_index_.emplace(next_object_id_, index);
    } catch (...) {
        // Уничтожаем лут если произошла ошибка
        loot.reset();
        loot_items_.pop_back();
        throw std::runtime_error("Failed to add new loot item to session");
    }
    //Creation successful
    ++next_object_id_;
}

void Session::AddRandomLootItems(size_t num_items) {
    for (int i = 0; i < num_items; ++i) {
        AddLootItem(
            model::GenRandomNum(map_->GetLootTypesSize()),
            ToGeomPt(map_->GetRandomRoadPt())
        );
    }
}

void Session::RemoveDog(model::DogPtr dog) {
    //TODO:
}

void Session::RemoveLootItem(model::GameObject::Id loot_item_id) {
    try {
        auto item_index = objects_index_.at(loot_item_id);
        auto item_ptr = loot_items_.at(item_index);

        item_ptr.reset();
        loot_items_.erase(loot_items_.begin() + item_index);
        ++deleted_objects_counter_;
    } catch (std::exception& ex) {
        //DEBUG
        std::cerr << "Failed to remove loot item: " << ex.what();
    }
}

void Session::AdvanceTime(model::TimeMs delta_t) {
    GenerateLoot(delta_t);
    MoveAllDogs(delta_t);
    ProcessCollisions();
}


//---------------------------------------------------------
void Session::MoveDog(const model::DogPtr& dog, model::TimeMs delta_t) const {
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

void Session::MoveAllDogs(model::TimeMs delta_t) const {
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
        if (event.is_loot_collect) {
            HandleCollision(loot_items_.at(event.obj_id), dogs_.at(event.dog_id));
        } else if (event.is_items_return) {
            HandleCollision(offices_.at(event.obj_id), dogs_.at(event.dog_id));
        }

    }
}

void Session::HandleCollision(const model::LootItemPtr& loot, const model::DogPtr& dog) {
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
PlayerManager::PlayerManager(const GamePtr& game)
    : game_(game) {
}

PlayerManager::PlayerManager(GamePtr&& game)
    : game_(std::move(game)) {
}

PlayerPtr PlayerManager::Add(model::DogPtr dog, SessionPtr session) {
    std::pair<TokenIt, bool> token_result{token_to_player_.end(), false};
    PlayerPtr player = nullptr;
    size_t index = players_.size();
    try {
        player = std::make_shared<Player>(next_player_id_, session, dog);
        players_.push_back(player);
        //update indices
        token_result = token_to_player_.emplace(std::move(GenerateToken()), index);
        player_to_token_[next_player_id_] = token_result.first;

        map_dog_id_to_player_index_[session->GetMapId()][dog->GetId()] = index;
    } catch (std::exception& ex) {
        player.reset();
        if (!players_.back()) {
            players_.pop_back();
        }
        if (token_result.first != token_to_player_.end()) {
            token_to_player_.erase(token_result.first);
        }
        //DEBUG
        std::cerr << "Failed to add player: " << ex.what();
        //throw to prevent token being sent with response
        throw std::runtime_error("Failed to add player to session");
    }
    //Success
    ++next_player_id_;
    return player;
}

PlayerPtr PlayerManager::GetPlayerByToken(const Token& token) const {
    auto it = token_to_player_.find(token);
    PlayerPtr player = nullptr;

    if (it != token_to_player_.end()) {
        player = players_.at(it->second);
    }
    return player;
}

PlayerPtr PlayerManager::GetPlayerByMapDogId(const model::Map::Id& map_id, size_t dog_id) const {
    if (!map_dog_id_to_player_index_.contains(map_id)
        || !map_dog_id_to_player_index_.at(map_id).contains(dog_id)) {
        return nullptr;
    }
    return players_.at(map_dog_id_to_player_index_.at(map_id).at(dog_id));
}

TokenPtr PlayerManager::GetToken(const PlayerPtr& player) const {
    auto it = player_to_token_.find(player->GetId());
    if (it == player_to_token_.end()) {
        return nullptr;
    }
    auto& token = (it->second)->first;
    return &token;
}

const PlayerManager::MapToSessions& PlayerManager::GetAllSessionsByMap() const {
    return map_to_sessions_;
}

SessionPtr PlayerManager::JoinOrCreateSession(const Map::Id& map_id) {
    //TODO: No _free_ session for map
    //Check if a session exixts on map. GetMapSessions checks that mapid is valid
    const auto [map_ptr, sessions_on_map] = GetMapSessions(map_id);
    if(!map_ptr || !sessions_on_map) {
        //DEBUG
        std::cerr << "Nullptr returned by GetMapSessions";
        return nullptr;
    }
    //No sessions on map, create new session
    if(sessions_on_map->empty()) {
        SessionPtr session = nullptr;
        auto session_it = sessions_on_map->end();

        try {
            session = std::make_shared<Session>(next_session_id_++, map_ptr, std::move(game_->GetSettings()));
            session_it = (sessions_on_map->emplace(session)).first;

            return session;
        } catch (std::exception& ex) {
            session.reset();
            if (session_it != sessions_on_map->end()) {
                sessions_on_map->erase(session_it);
            }
            //DEBUG
            std::cerr << "Failed to create new session on map: " << ex.what();
            throw std::runtime_error("Failed to make a new session on map");
        }
    }
    //A session exists and can be joined
    return JoinFreeSession(sessions_on_map);
}

ConstSessionPtr PlayerManager::GetPlayerGameSession(const PlayerPtr& player) {
    return player->GetSession();
}

std::vector<PlayerPtr> PlayerManager::GetAllPlayersInSession(const PlayerPtr& player) const {
    std::vector<PlayerPtr> result;

    auto player_session = player->GetSession();
    const auto& map_id = player_session->GetMapId();

    for (const auto& dog : player_session->GetDogs()) {
        result.push_back(GetPlayerByMapDogId(map_id, dog->GetId()));
    }
    return result;
}

const Session::LootItems &PlayerManager::GetSessionLootList(const PlayerPtr& player) {
    return player->GetSession()->GetLootItems();
}

PlayerManager::MapAndSessions PlayerManager::GetMapSessions(const Map::Id& map_id) {
    //Check that map id is valid
    auto map_ptr = game_->FindMap(map_id);
    if(!map_ptr) {
        //DEBUG
        std::cerr << "Attempt to get session, map not found";
        return {nullptr, nullptr};
    }
    //if no session on map, create & return empty set
    return {map_ptr, &map_to_sessions_[map_id]};
}

SessionPtr PlayerManager::JoinFreeSession(const MapSessions* map_sessions_ptr) {
    return *map_sessions_ptr->begin();
}


//=================================================
//============= GameInterface =====================
GameInterface::GameInterface(const GamePtr& game_ptr)
    : game_(game_ptr)
      , player_manager_(game_) {
}

ConstSessionPtr GameInterface::GetSession(const PlayerPtr& player) const {
    return player_manager_.GetPlayerGameSession(player);
}

std::vector<app::PlayerPtr> GameInterface::GetPlayerList(const PlayerPtr& player) const {
    return player_manager_.GetAllPlayersInSession(player);
}

const Session::LootItems &GameInterface::GetLootList(const PlayerPtr& player) const {
    return player_manager_.GetSessionLootList(player);
}

PlayerPtr GameInterface::FindPlayerByToken(const Token& token) const {
    return player_manager_.GetPlayerByToken(token);
}

JoinGameResult GameInterface::JoinGame(std::string_view map_id_str, std::string_view player_dog_name) {
    model::Map::Id map_id{std::move(std::string(map_id_str))};

    auto session = player_manager_.JoinOrCreateSession(map_id);
    auto dog = session->AddDog(std::move(std::string(player_dog_name)));
    auto player = player_manager_.Add(dog, session);

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

void GameInterface::AdvanceGameTime(model::TimeMs delta_t) const {
    const auto& sessions_by_map = player_manager_.GetAllSessionsByMap();
    for (auto& [map, sessions] : sessions_by_map) {

        //TODO: Atm. there is only one session per map
        for(auto& session : sessions) {
            session->AdvanceTime(delta_t);
        }
    }
}

bool GameInterface::MoveCommandValid(const char move_command) {
    return !isblank(move_command)
           && valid_move_chars_.find(move_command) != valid_move_chars_.npos;
}
} //namespace app

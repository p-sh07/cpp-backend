//
// Created by ps on 8/22/24.
//

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "application.h"
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
Session::Session(size_t id, MapPtr map, gamedata::Settings settings)
    : id_(id)
    , map_(map)
    , settings_(std::move(settings))
    , loot_generator_(settings_.loot_gen_interval, settings_.loot_gen_prob) {

    if (!map) {
        throw std::runtime_error("map nullptr passed to Session constructor");
    }
    //Set map bag & speed settings if specified
    settings_.map_dog_speed    = map_->GetDogSpeed();
    settings_.map_bag_capacity = map_->GetBagCapacity();

    AddOffices(map_->GetOffices());
}

size_t Session::GetId() const {
    return id_;
}

model::ConstMapPtr Session::GetMap() const {
    return map_;
}

model::TimeMs Session::GetTime() const {
    return session_time_;
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

DogPtr Session::GetDog(Dog::Id id) {
    const auto& dog_it = dogs_.find(id);
    return dog_it == dogs_.end()
               ? nullptr
               : &(dog_it->second);
}

ConstDogPtr Session::GetDog(Dog::Id id) const {
    const auto& dog_it = dogs_.find(id);
    return dog_it == dogs_.end()
               ? nullptr
               : &(dog_it->second);
}

const Session::LootItems &Session::GetLootItems() const {
    return loot_items_;
}

DogPtr Session::AddDog(Dog dog) {
    auto dog_id                      = dog.GetId();
    const auto [dog_map_it, success] = dogs_.emplace(dog_id, std::move(dog));

    //Dog with this id already exists
    if (!success) {
        if (dog_map_it->second.GetTag() != dog.GetTag()) {
            throw std::runtime_error("Different dog with this id already exists");
        }
        return &dog_map_it->second;
    }
    return gatherers_.emplace_back(&dog_map_it->second);
}

//---------------------------------------------------------
DogPtr Session::AddDog(Dog::Id id, const Dog::Tag& name) {
    const auto starting_pos = settings_.randomised_dog_spawn
                                  ? map_->GetRandomRoadPt()
                                  : map_->GetFirstRoadPt();

    const auto [dog_map_it, success] = dogs_.emplace(id,
                                                     Dog{
                                                         id, starting_pos, settings_.dog_width
                                                         , name, settings_.GetBagCap()
                                                     });

    if (!success) {
        if (dog_map_it->second.GetTag() != name) {
            throw std::runtime_error("Different dog with this id already exists");
        }
        return &dog_map_it->second;
    }
    //update gatherers index
    return gatherers_.emplace_back(&dog_map_it->second);
}

void Session::AddLootItem(LootItem::Id id, LootItem::Type type, model::Point2D pos) {
    loot_items_.emplace_back(std::make_shared<LootItem>(
        id, pos, settings_.loot_item_width, type, map_->GetLootItemValue(type)
    ));
}

void Session::AddRandomLootItems(size_t num_items) {
    for (int i = 0; i < num_items; ++i) {
        AddLootItem(
            next_object_id_,
            model::GenRandomNum(map_->GetLootTypesSize()),
            map_->GetRandomRoadPt()
        );
        ++next_object_id_;
    }
}

void Session::RemoveDog(Dog::Id dog_id) {
    dogs_.erase(dog_id);
    gatherers_.erase(
        std::ranges::find_if(gatherers_, [dog_id](const DogPtr& dog_ptr) {
            return dog_ptr->GetId() == dog_id;
        })
    );
}

void Session::RemoveLootItem(GameObject::Id loot_item_id) {

    auto it = std::ranges::find_if(loot_items_, [loot_item_id](const LootItemPtr& item) {
        return item->GetId() == loot_item_id;
    });

    //Delete item and remove from deque
    it->reset();
    loot_items_.erase(it);
}

void Session::AdvanceTime(model::TimeMs delta_t) {
    session_time_ += delta_t;

    MoveAllDogs(delta_t);
    ProcessCollisions();

    //Generate loot after, so that a loot item is not randomly picked up by dog
    GenerateLoot(delta_t);
}

void Session::AddOffices(const Map::Offices& offices) {
    for (const auto& office : map_->GetOffices()) {
        const auto& off_ptr = offices_.emplace_back(std::make_shared<ItemsReturnPoint>(
            next_object_id_++, office, settings_.office_width
        ));

        objects_.emplace_back(off_ptr);
    }
}

//---------------------------------------------------------
void Session::MoveDog(Dog& dog, model::TimeMs delta_t) {
    //Operators for time-distance calculations
    using model::operator+;
    using model::operator*;

    model::Point2D max_move_pt  = dog.GetPos() + dog.GetSpeed() * delta_t;
    Map::MoveResult move_result = map_->ComputeRoadMove(dog.GetPos(), max_move_pt);
    if (move_result.road_edge_reached_) {
        dog.Stop();
    }
    dog.SetPos(move_result.dst);
}

void Session::MoveAllDogs(model::TimeMs delta_t) {
    for (auto& [id, dog] : dogs_) {
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
    try {
        //NB: this used to be a member of Session, but recent changes introduced bugs:
        // - refs to object/gatherer container were becoming invalidates - could be due to std::move or unordered_map/rehash?
        model::CollisionDetector collision_detector{objects_, gatherers_};
        auto collision_events = collision_detector.FindCollisions();

        for (const auto& event : collision_events) {
            const auto& dog = gatherers_.at(event.gatherer_id);
            dog->ProcessCollision(objects_.at(event.item_id));
        }
    } catch (...) {
        std::cerr << "collision detection error";
    }
}

//=================================================
//=================== Player ======================
Player::Player(size_t id, SessionPtr session, DogPtr dog)
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

PlayerPtr PlayerSessionManager::CreatePlayer(const Map::Id& map, const Dog::Tag& dog_tag) {
    auto session = JoinOrCreateSession(next_session_id_++, map);
    auto dog     = session->AddDog(next_dog_id_++, dog_tag);
    return AddPlayer(next_player_id_++, dog, session, GenerateToken());
}

PlayerPtr PlayerSessionManager::AddPlayer(Player::Id id, DogPtr dog, SessionPtr session, Token token) {
    //TODO: check for duplicate token? Catch error and pop back player?
    const auto [player_it, success] = players_.emplace(id, Player{id, session, dog});

    //update indices
    auto token_result = token_to_player_.emplace(std::move(token), id);
    player_to_token_[id] = token_result.first;
    map_dog_id_to_player_index_[session->GetMapId()][dog->GetId()] = id;

    //Success;
    return &player_it->second;
}

//Make sure to use this function only after restoring dog and session
PlayerPtr PlayerSessionManager::RestorePlayer(Player::Id id, Dog::Id dog_id, Session::Id session_id, Token token) {
    try {
        SessionPtr session = &sessions_.at(session_id);

        //Update next_session id during restore
        if (next_session_id_ <= session_id) {
            next_session_id_ = session_id + 1;
        }

        DogPtr dog = session->GetDog(dog_id);
        if (!dog) {
            throw std::out_of_range("dog not found by id");
        }

        //update latest player & dog id
        if (next_player_id_ <= id) {
            next_player_id_ = id + 1;
        }

        if (next_dog_id_ <= dog_id) {
            next_dog_id_ = dog_id + 1;
        }

        return AddPlayer(id, dog, session, std::move(token));
    } catch (std::exception& ex) {
        //DEBUG
        std::cerr << "failed to restore player: " << id << ", exc.: " << ex.what() << std::endl;
        return nullptr;
    }
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

TokenPtr PlayerSessionManager::GetToken(const Player::Id player_id) const {
    auto token_it = player_to_token_.find(player_id);
    return token_it == player_to_token_.end()
               ? nullptr
               : &token_it->second->first;
}

TokenPtr PlayerSessionManager::GetToken(ConstPlayerPtr player) const {
    return GetToken(player->GetId());
}

const PlayerSessionManager::Players &PlayerSessionManager::GetAllPlayers() const {
    return players_;
}

const PlayerSessionManager::Sessions &PlayerSessionManager::GetAllSessions() const {
    return sessions_;
}

SessionPtr PlayerSessionManager::JoinOrCreateSession(Session::Id session_id, const Map::Id& map_id) {
    //Check if a session exixts on map. GetMapSessions checks that mapid is valid
    const auto map_ptr = game_->FindMap(map_id);
    if (!map_ptr) {
        //DEBUG
        std::cerr << "Cannot join session, map not found";
        return nullptr;
    }
    //No sessions on map, create new session
    auto existing_session_it = map_to_session_index_.find(map_id);
    if (existing_session_it == map_to_session_index_.end()) {
        const auto& [session_it, success] = sessions_.emplace(session_id,
                                                              Session{session_id, map_ptr, game_->GetSettings()}
        );

        try {
            map_to_session_index_.emplace(map_id, session_id);

            //if successfully emplaced session, increment session id
            return &session_it->second;
        } catch (std::exception& ex) {
            //DEBUG
            std::cerr << "Failed to create new session on map: " << ex.what();
            throw std::runtime_error("Failed to make a new session on map");
        }
    }
    //A session exists and can be joined
    return &sessions_.at(existing_session_it->second);
}

ConstSessionPtr PlayerSessionManager::GetPlayerGameSession(ConstPlayerPtr player) {
    return player->GetSession();
}

std::vector<ConstPlayerPtr> PlayerSessionManager::GetAllPlayersInSession(ConstPlayerPtr player) const {
    std::vector<ConstPlayerPtr> result;

    auto player_session = player->GetSession();
    const auto& map_id  = player_session->GetMapId();

    for (const auto& [dog_id, _] : player_session->GetDogs()) {
        std::cerr << dog_id << std::endl;
        result.push_back(GetPlayerByMapDogId(map_id, dog_id));
    }
    return result;
}

const Session::LootItems &PlayerSessionManager::GetSessionLootList(ConstPlayerPtr player) {
    return player->GetSession()->GetLootItems();
}

void PlayerSessionManager::AdvanceTime(model::TimeMs delta_t) {
    for (auto& [_, session] : sessions_) {
        session.AdvanceTime(delta_t);
    }
}


//=================================================
//============= GameInterface =====================
GameInterface::GameInterface(net::io_context& io, const GamePtr& game_ptr, const AppListenerPtr& app_listener_ptr)
    : io_(io)
    , game_(game_ptr)
    , app_listener_(app_listener_ptr)
    , player_manager_(app_listener_ptr ? app_listener_ptr->Restore(game_) : PlayerSessionManager{game_}) {
}

ConstSessionPtr GameInterface::GetSession(ConstPlayerPtr player) const {
    return player_manager_.GetPlayerGameSession(player);
}

std::vector<ConstPlayerPtr> GameInterface::GetPlayerList(ConstPlayerPtr player) const {
    return player_manager_.GetAllPlayersInSession(player);
}

const Session::LootItems &GameInterface::GetLootList(ConstPlayerPtr player) const {
    return player_manager_.GetSessionLootList(player);
}

ConstPlayerPtr GameInterface::FindPlayerByToken(const Token& token) const {
    return player_manager_.GetPlayerByToken(token);
}

JoinGameResult GameInterface::JoinGame(std::string map_id_str, std::string player_dog_name) {
    //TODO: make a new strand for a new session
    //auto api_strand = net::make_strand(ioc);

    const Map::Id map_id{std::move(map_id_str)};
    const Dog::Tag dog_tag{std::move(player_dog_name)};

    const auto player = player_manager_.CreatePlayer(map_id, dog_tag);

    // <-Make response, send player token
    auto token = player_manager_.GetToken(player);
    return {player->GetId(), token};
}

void GameInterface::SetPlayerMovement(ConstPlayerPtr player, const char move_command) {
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
    try {
        if (app_listener_) {
            app_listener_->OnTick(delta_t, player_manager_);
        }
    } catch (std::exception& ex) {
        //TODO: Logger
        std::cerr << "serialization error occured: " << ex.what() << std::endl;
    }
}

bool GameInterface::MoveCommandValid(const char move_command) const {
    const auto& valid_move_chars = game_->GetSettings().valid_move_chars;
    return !isblank(move_command)
           && valid_move_chars.find(move_command) != valid_move_chars_.npos;
}
} //namespace app

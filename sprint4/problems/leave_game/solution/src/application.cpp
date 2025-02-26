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
using namespace std::literals;
namespace rg = std::ranges;
namespace vw = std::views;

size_t TokenHasher::operator()(const Token& token) const {
    auto str_hasher = std::hash<std::string>();
    return str_hasher(*token);
}

//=================================================
//=================== Session =====================
Session::Session(size_t id, MapPtr map, gamedata::Settings settings/*, std::shared_ptr<database::Listener> db_listener*/)
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
        if (dog_map_it->second.GetName() != dog.GetName()) {
            throw std::runtime_error("Different dog with this id already exists");
        }
        return &dog_map_it->second;
    }
    return gatherers_.emplace_back(&dog_map_it->second);
}

//---------------------------------------------------------
DogPtr Session::AddDog(Dog::Id id, std::string name) {
    const auto starting_pos = settings_.randomised_dog_spawn
                                  ? map_->GetRandomRoadPt()
                                  : map_->GetFirstRoadPt();

    const auto [dog_map_it, success] = dogs_.emplace(id,
         Dog{ id, starting_pos, settings_.dog_width
           , std::move(name), settings_.GetBagCap()
    });

    if (!success) {
        if (dog_map_it->second.GetName() != name) {
            throw std::runtime_error("Different dog with this id already exists");
        }
        return &dog_map_it->second;
    }
    //Update default dog expiry time
    &dog_map_it->second.SetRetireTime(settings_.default_inactivity_timeout_ms_);

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

std::vector<Dog::Id> Session::AdvanceTime(model::TimeMs delta_t) {
    session_time_ += delta_t;

    // std::cerr << std::endl << "moving dogs...\n";

    MoveAllDogs(delta_t);

    // std::cerr << "collisions"  << std::endl;
    ProcessCollisions();

    // std::cerr << "loot" << std::endl;
    //Generate loot after, so that a loot item is not randomly picked up by dog
    GenerateLoot(delta_t);

    std::cerr << "retired dogs" << std::endl;
    return GetRetiredDogs();
}

// void Session::EraseDog(Dog::Id dog_id) {
//     dogs_.erase(dog_id);
// }

void Session::AddOffices(const Map::Offices& offices) {
    for (const auto& office : map_->GetOffices()) {
        const auto& off_ptr = offices_.emplace_back(std::make_shared<ItemsReturnPoint>(
            next_object_id_++, office, settings_.office_width
        ));

        objects_.emplace_back(off_ptr);
    }
}

//---------------------------------------------------------
void Session::MoveDog(Dog& dog, model::TimeMs delta_t) const {
    //Operators for time-distance calculations
    using model::operator+;
    using model::operator*;

    // std::cerr << " -> computing max move\n";
    model::Point2D max_move_pt  = dog.GetPos() + dog.GetSpeed() * delta_t;

    // std::cerr << " -> computing dog pos\n";
    Map::MoveResult move_result = map_->ComputeRoadMove(dog.GetPos(), max_move_pt);

    if (move_result.road_edge_reached_) {
        dog.Stop();
    }
    //If dog had no movement during this tick, it will continue to add expiration time
    dog.SetPos(move_result.dst);
    dog.AddTime(delta_t);
}

void Session::MoveAllDogs(model::TimeMs delta_t) {
    // std::cerr << "=inside MoveAllDogs, dogs sz = " << dogs_.size() << '\n';
    for (auto& dog : dogs_ | vw::values) {
        // std::cerr << "moving dog " << std::endl; 
        // std::cerr << dog.GetId() << "...\n";
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
        // - refs to object/gatherer container were becoming invalidated - could be due to std::move or unordered_map/rehash?
        // std::cerr << " -> init detector" << std::endl;
        model::CollisionDetector collision_detector{objects_, gatherers_};

        // std::cerr << " -> find collisions" << std::endl; 
        auto collision_events = collision_detector.FindCollisions();

        // std::cerr << "process:" << std::endl; 
        for (const auto& event : collision_events) {
            // std::cerr << " -> processing: gath.sz = " << gatherers_.size();
            // std::cerr << " id = " << event.gatherer_id << std::endl;
            const auto& dog = gatherers_.at(event.gatherer_id);
            if(dog) {
                // std::cerr << " --> going into dog: " << dog->GetId() << std::endl;
                dog->ProcessCollision(objects_.at(event.item_id));
            } else {
                std::cerr << "nullptr gatherer!" << std::endl;
                //gatherers_.erase(event.gatherer_id);
            }
            
        }
    } catch (...) {
        std::cerr << "collision detection error";
    }
}

std::vector<Dog::Id> Session::GetRetiredDogs() {
    std::vector<Dog::Id> retired;
    retired.reserve(dogs_.size());
    // std::cerr << " -> dogs.sz = " << dogs_.size() << std::endl;
    for(const auto&[id, dog] : dogs_) {
        // std::cerr << " --> proc. dog: " << id << '\n';
        if(dog.IsExpired()) {
            // std::cerr << " --> ins. dog: " << id << '\n';
            retired.push_back(id);
        }
    }
    return retired;
}

// void Session::ClearRetiredDogs() {
//     //no parameter version: erase all expired dogs
//     std::erase_if(dogs_, [](const auto& item) {
//         return item.second.IsExpired();
//     });
// }

void Session::EraseRetiredDogs(const std::vector<Dog::Id>& retired_dog_ids) {
    rg::for_each(retired_dog_ids, [&](const auto dog_id) {
        RemoveDog(dog_id);
    });
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

PlayerSessionManager::PlayerSessionManager(const GamePtr& game, Sessions sessions): game_(game)
    , sessions_(std::move(sessions)) {
    //update indices
    for(const auto& [sess_id, session] : sessions_) {
        map_to_session_index_[session.GetMap()->GetId()] = sess_id;
    }

}

PlayerPtr PlayerSessionManager::CreatePlayer(const Map::Id& map, std::string dog_name) {
    auto session = JoinOrCreateSession(next_session_id_++, map);
    auto dog     = session->AddDog(next_dog_id_++, dog_name);
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

void PlayerSessionManager::RemoveSession(Session::Id session_id) {
    auto sess_it = sessions_.find(session_id);
    if(sess_it == sessions_.end()) {
        return;
    }
    auto sess_map_id = sess_it->second.GetMap()->GetId();

    map_to_session_index_.erase(sess_map_id);
    sessions_.erase(sess_it);
}

ConstSessionPtr PlayerSessionManager::GetPlayerGameSession(ConstPlayerPtr player) {
    return player->GetSession();
}

std::vector<ConstPlayerPtr> PlayerSessionManager::GetAllPlayersInSession(ConstPlayerPtr player) const {
    std::vector<ConstPlayerPtr> result;

    auto player_session = player->GetSession();
    const auto& map_id  = player_session->GetMapId();

    for (const auto& dog_id : player_session->GetDogs() | vw::keys) {
        // std::cerr << dog_id << std::endl;
        result.push_back(GetPlayerByMapDogId(map_id, dog_id));
    }
    return result;
}

const Session::LootItems &PlayerSessionManager::GetSessionLootList(ConstPlayerPtr player) {
    return player->GetSession()->GetLootItems();
}

gamedata::PlayerStats PlayerSessionManager::RetirePlayer(const Map::Id map_id, Dog::Id dog_id) {
    const auto player      = GetPlayerByMapDogId(map_id, dog_id);
    if(!player) {
        //DEBUG
        std::cerr << "Player nullptr in retire!" << std::endl;
        throw std::runtime_error("");
    }

    const auto dog         = player->GetDog();
    if(!dog) {
        //DEBUG
        std::cerr << "Dog nullptr in retire!" << std::endl;
        throw std::runtime_error("");
    }

    const auto player_sess = player->GetSession();
    if(!player_sess) {
        //DEBUG
        std::cerr << "Session nullptr in retire!" << std::endl;
        throw std::runtime_error("");
    }

    gamedata::PlayerStats stats{
        dog->GetName(),
        dog->GetScore(),
        dog->GetIngameTime().count()
    };
    //std::cerr << "retiring player: " << player->GetId() << ", " << dog->GetName() << " after: " << dog->GetIngameTime().count() / 1000.0 << " sec" << std::endl;
    //Erase player's dog
    player_sess->RemoveDog(dog_id);

    //Erase player's token:
    auto token_it = player_to_token_.at(player->GetId());
    token_to_player_.erase(token_it);
    player_to_token_.erase(player->GetId());

    //Erase player
    players_.erase(player->GetId());

    //Erase empty session?
    if(player_sess->GetDogCount() == 0) {
        RemoveSession(player_sess->GetId());
    }

    return stats;
}

std::vector<gamedata::PlayerStats> PlayerSessionManager::AdvanceTime(model::TimeMs delta_t) {
    std::vector<gamedata::PlayerStats> retired_players;

    for (auto& session : sessions_ | vw::values) {
        auto map_id = session.GetMapId();
        auto retired_dogs = session.AdvanceTime(delta_t);

        rg::for_each(retired_dogs, [&](const auto& dog_id) {
            retired_players.emplace_back(RetirePlayer(map_id, dog_id));
        });
    }
    return retired_players;
}


//=================================================
//============= GameInterface =====================
GameInterface::GameInterface(net::io_context& io, GamePtr game_ptr, AppListenerPtr app_listener_ptr, database::PlayerStats& player_db)
    : io_(io)
    , game_(std::move(game_ptr))
    , app_listener_(std::move(app_listener_ptr))
    , player_manager_(app_listener_ptr ? app_listener_ptr->Restore(game_) : PlayerSessionManager{game_})
    , player_stat_db_(player_db){
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

std::vector<gamedata::PlayerStats> GameInterface::GetPlayerStats(std::optional<size_t> start, std::optional<size_t> max_players) {
    return player_stat_db_.LoadPlayersStats(std::move(start), std::move(max_players));
}

ConstPlayerPtr GameInterface::FindPlayerByToken(const Token& token) const {
    return player_manager_.GetPlayerByToken(token);
}

void GameInterface::RestorePlayerManagerState(PlayerSessionManager psm) {
    player_manager_ = std::move(psm);
}

JoinGameResult GameInterface::JoinGame(std::string map_id_str, std::string player_dog_name) {
    //TODO: make a new strand for a new session
    //auto api_strand = net::make_strand(ioc);

    const Map::Id map_id{std::move(map_id_str)};
    const auto player = player_manager_.CreatePlayer(std::move(map_id), std::move(player_dog_name));

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
    auto retired_players = player_manager_.AdvanceTime(delta_t);
    try {
        if (app_listener_) {
            app_listener_->OnTick(delta_t, player_manager_);
        }
        //TODO: add listener for player retirement?
    } catch (std::exception& ex) {
        //TODO: Logger
        // std::cerr << "serialization error occured: " << ex.what() << std::endl;
    }
    //TODO: Dispatch db write to io
    std::cerr << "writing to db...\n";
    player_stat_db_.SavePlayersStats(retired_players);
}

const PlayerSessionManager& GameInterface::GetPlayerManager() const {
    return player_manager_;
}

bool GameInterface::MoveCommandValid(const char move_command) const {
    const auto& valid_move_chars = game_->GetSettings().valid_move_chars;
    return valid_move_chars.find(move_command) != valid_move_chars_.npos;
}
} //namespace app

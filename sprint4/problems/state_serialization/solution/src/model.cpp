#include "model.h"

#include <stdexcept>

//DEBUG
#include <iostream>
#include <boost/log/exceptions.hpp>
//#define GATHER_DEBUG

namespace model {
using namespace std::literals;

//TODO: change value depending on accuracy
static constexpr double EPSILON = 0.0000000001;

//==== Random int generator for use in game model
int GenRandomNum(int limit1, int limit2) {
    ///'static' allows to reuse generator in the same thread, since expencive to init
    static thread_local std::mt19937 gen_local{std::random_device()()};

    //when min max are swapped:
    size_t min = std::min(limit1, limit2);
    size_t max = std::max(limit1, limit2);

    //Since using with size(), Generate numbers in non-inclusive range [min, max)
    if(max > 0) {
        --max;
    }
    ///distribution is cheap to construct, so no need to reuse
    std::uniform_int_distribution<int> distr(min,max);
    return distr(gen_local);
}

size_t GenRandomNum(size_t limit1, size_t limit2) {
    int a = static_cast<int>(limit1), b = static_cast<int>(limit2);
    return static_cast<size_t>(GenRandomNum(a,b));
}

//=================================================
//=================== Road ========================
Road::Road(Road::VerticalTag, Point start, Coord end_y)
    : start_{start}
    , end_{start.x, end_y} {
}

Road::Road(Road::HorizontalTag, Point start, Coord end_x)
    : start_{start}
    , end_{end_x, start.y} {
}

bool Road::IsHorizontal() const {
    return start_.y == end_.y;
}

bool Road::IsVertical() const {
    return !IsHorizontal();
}

Point Road::GetStart() const {
    return start_;
}

Point Road::GetEnd() const {
    return end_;
}

Point Road::GetRandomPt() const {
    if(IsHorizontal()) {
        auto rand_x = GenRandomNum(start_.x, end_.x);
        Point pt{static_cast<Coord>(rand_x), start_.y};
        std::cerr << "selecting random between: " << start_.x << " & " << end_.x << " == " << rand_x << '\n';
        return pt;
    }
    auto rand_y = GenRandomNum(start_.y, end_.y);
    Point pt{start_.x, static_cast<Coord>(rand_y)};
    std::cerr << "selecting random between: " << start_.y << " & " << end_.y << " == " << rand_y << '\n';
    return pt;
    //return {static_cast<Coord>(start_.x, util::random_num(start_.y, end_.y))};
}

//=================================================
//=================== Building ====================
Building::Building(Rectangle bounds)
    : bounds_{bounds} {
}

const Rectangle& Building::GetBounds() const {
    return bounds_;
}

//=================================================
//=================== Office ====================
Office::Office(Office::Id id, Point position, Offset offset)
    : id_{std::move(id)}
    , position_{position}
    , offset_{offset} {
}

const Office::Id& Office::GetId() const {
    return id_;
}

Point Office::GetPosition() const {
    return position_;
}

Offset Office::GetOffset() const {
    return offset_;
}

double Office::GetWidth() const {
    return width_;
}

//=================================================
//================ Map ============================
void Map::AddOffice(Office office) {
    if(warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch(...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

Point Map::GetRandomRoadPt() const {
    auto& random_road = roads_.at(GenRandomNum(roads_.size()));
    return random_road.GetRandomPt();
}

Point Map::GetFirstRoadPt() const {
    return roads_.at(0).GetStart();
}

Map::Map(Map::Id id, std::string name)
    : id_(std::move(id))
    , name_(std::move(name)) {
}

const Map::Id& Map::GetId() const {
    return id_;
}

const std::string& Map::GetName() const {
    return name_;
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

const Map::Buildings& Map::GetBuildings() const {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const {
    return roads_;
}

const Map::Offices& Map::GetOffices() const {
    return offices_;
}

size_t Map::GetLootItemValue(LootItem::Type type) const {
    return loot_types_->GetItemValue(type);
}

void Map::AddRoad(const Road& road) {
    auto rd = roads_.emplace_back(road);

    //store road start points in index
    RoadXtoIndex_[rd.GetStart().x].insert(roads_.size() - 1);
    RoadYtoIndex_[rd.GetStart().y].insert(roads_.size() - 1);
}

std::optional<double> Map::GetDogSpeed() const {
    return dog_speed_;
}

void Map::SetDogSpeed(double speed) {
    dog_speed_ = speed;
}

const Road* Map::FindVertRoad(Point2D pt) const {
    auto it = RoadXtoIndex_.find(static_cast<Coord>(std::round(pt.x)));
    if(it == RoadXtoIndex_.end()) {
        return nullptr;
    }

    auto pt_is_on_road = [&](const Road* rd, const Point2D& pt) {
        Point check{static_cast<Coord>(std::round(pt.x)), static_cast<Coord>(std::round(pt.y))};

        // for equal x's, check if y is within the vertical road
        return (check.y >= std::min(rd->GetStart().y, rd->GetEnd().y))
            || (std::max(rd->GetStart().y, rd->GetEnd().x) >= check.y);
    };

    for(const auto road_idx : it->second) {
        const auto road = &roads_[road_idx];
        //skip Horizontal roads
        if(road->IsVertical() && pt_is_on_road(road, pt)) {
            return &roads_[road_idx];
        }
    }
    return nullptr;
}

const Road* Map::FindHorRoad(Point2D pt) const {
    auto pt_is_on_road = [&](const Road* rd, const Point2D& pt) {
        Point check{static_cast<Coord>(std::round(pt.x)), static_cast<Coord>(std::round(pt.y))};
        return (check.x >= std::min(rd->GetStart().x, rd->GetEnd().x))
            && (std::max(rd->GetStart().x, rd->GetEnd().x) >= check.x);
    };

    auto it = RoadYtoIndex_.find(static_cast<Coord>(std::round(pt.y)));
    if(it == RoadYtoIndex_.end()) {
        return nullptr;
    }

    for(const auto road_idx : it->second) {
        const auto road = &roads_[road_idx];

        //skip vertical roads
        if(road->IsHorizontal() && pt_is_on_road(road, pt)) {
            return &roads_[road_idx];
        }
    }
    return nullptr;
}

LootItem::Type Map::GetRandomLootTypeNum() const {
    auto rand_type = GenRandomNum(loot_types_->Size());
    // assert(rand_type >= 0 && rand_type < loot_types_->Size());
    return static_cast<LootItem::Type>(rand_type);
}

const gamedata::LootTypeInfo* Map::GetLootInfo() const {
    return loot_types_.get();
}

std::optional<size_t> Map::GetBagCapacity() const {
    return bag_capacity_;
}

void Map::SetBagCapacity(size_t cap) {
    bag_capacity_ = cap;
}

//=================================================
//=================== Dog =========================
Dog::Dog(size_t id, std::string name, Point2D position, size_t bag_capacity)
    : id_(id)
    , name_(name)
    , pos_(position)
    , bag_capacity_(bag_capacity) {
}

std::string_view Dog::GetName() const {
    return name_;
}

size_t Dog::GetId() const {
    return id_;
}

Point2D Dog::GetPos() const {
    return pos_;
}

Point2D Dog::GetPrevPos() const {
    return prev_pos_;
}

Speed Dog::GetSpeed() const {
    return speed_;
}

Dir Dog::GetDir() const {
    return direction_;
}

Dog& Dog::Stop() {
    speed_ = {0, 0};
    return *this;
}

Dog& Dog::SetSpeed(Speed sp) {
    speed_ = sp;
    return *this;
}

Dog& Dog::SetDir(Dir dir) {
    direction_ = dir;
    return *this;
}

Dog& Dog::SetMove(Dir dir, double speed_value) {
    direction_ = dir;
    switch(dir) {
        case Dir::NORTH:SetSpeed({0.0, -speed_value});
            break;
        case Dir::WEST:SetSpeed({-speed_value, 0.0});
            break;
        case Dir::SOUTH:SetSpeed({0, speed_value});
            break;
        case Dir::EAST:SetSpeed({speed_value, 0.0});
            break;

        default:SetSpeed({0.0, 0.0});
            break;
    }
    return *this;
}

Dog& Dog::AddScore(Score points) {
    score_ += points;
    return *this;
}

Dog& Dog::SetScore(Score total) {
    score_ = total;
    return *this;
}

Dog& Dog::SetPos(Point2D pos) {
    prev_pos_ = pos_;
    pos_ = pos;
    return *this;
}

Dog& Dog::SetBagCap(size_t capacity) {
    bag_capacity_ = capacity;
    return *this;
}

Point2D Dog::ComputeMaxMove(TimeMs delta_t) const {
    //TODO: check this
    if(speed_.x < EPSILON && speed_.y < EPSILON) {
        return pos_;
    }

    //This converts time to seconds
    double delta_t_sec = std::chrono::duration<double>(delta_t).count();
    return {pos_ + delta_t_sec * speed_};
}

Score Dog::GetScore() const {
    return score_;
}

bool Dog::CollectLootItem(LootItem& item) {
    if(BagIsFull() && item.collected) {
        return false;
    }
    bag_.push_back(
        {item.id, item.type}
    );
    item.collected = true;
    return true;
}

void Dog::UnloadAllItems() {
    bag_.clear();
}

bool Dog::BagIsFull() const {
    return bag_.size() >= bag_capacity_;
}

const Dog::Bag& Dog::GetBagItems() const {
    return bag_;
}

//=================================================
//=================== Session =====================
Session::Session(Id id, const Map& map, gamedata::Settings settings)
    : id_(id)
    , map_(map)
    , settings_(std::move(settings))
    , loot_generator_(settings_.loot_gen_interval, settings_.loot_gen_prob) {
}

[[maybe_unused]] Dog* Session::AddDog(Dog::Id id, Dog::Name name, Point2D pos) {
    auto [dog_it, success] = dogs_.emplace(
        id,
        Dog{id, std::move(name), pos, GetDogBagCapacity()}
    );

    //Failed to add dog
    if(!success) {
        //TODO: Throw?
        return nullptr;
    }

    return &(dog_it->second);
}

[[maybe_unused]] LootItem* Session::AddLootItem(LootItem::Id id, LootItem::Type type, Point2D pos) {
    auto item_score_value = map_.GetLootItemValue(type);

    auto [item_it, success] = loot_items_.emplace(
        id,
        LootItem{id, type, item_score_value, pos}
    );

    if(!success) {
        //TODO: Throw?
        return nullptr;
    }

    return &(item_it->second);
}

[[maybe_unused]] Dog* Session::AddDog(std::string name) {
    return AddDog(next_dog_id_++, std::move(name), GetSpawnPoint());
}

[[maybe_unused]] LootItem* Session::AddLootItem(LootItem::Type type) {
    return AddLootItem(next_loot_id_++, type, map_.GetRandomRoadPt());
}

void Session::AddRandomLootItems(size_t num_items) {
    for(int i = 0; i < num_items; ++i) {
        AddLootItem(map_.GetRandomLootTypeNum());
    }
}

size_t Session::GetId() const {
    return id_;
}

const Map::Id& Session::GetMapId() const {
    return map_.GetId();
}

const Session::Dogs& Session::GetAllDogs() const {
    return dogs_;
}

const Session::LootItems& Session::GetLootItems() const {
    return loot_items_;
}

void Session::AdvanceTime(TimeMs delta_t) {
    GenerateLoot(delta_t);
    MoveAllDogs(delta_t);
    ProcessCollisions(delta_t);
}

Point2D Session::ComputeDogMove(Dog& dog, const Road* road, TimeMs delta_t) {
    //Maximum point dog can reach in delta_t if no road limit is hit
    auto max_move = dog.ComputeMaxMove(delta_t);
    switch(dog.GetDir()) {
        case Dir::NORTH: {
            auto road_limit_y = 1.0 * std::min(road->GetStart().y, road->GetEnd().y) - 0.4;

            //Choose the least value (closest to starting point, it is the limit)
            auto move_y_dist = std::max(road_limit_y, max_move.y);

            if(move_y_dist == road_limit_y) {
                dog.Stop();
            }
            return {dog.GetPos().x, move_y_dist};
        }
        case Dir::SOUTH: {
            auto road_limit_y = 1.0 * std::max(road->GetStart().y, road->GetEnd().y) + 0.4;
            auto move_y_dist = std::min(road_limit_y, max_move.y);

            if(move_y_dist == road_limit_y) {
                dog.Stop();
            }
            return {dog.GetPos().x, move_y_dist};
        }
        case Dir::WEST: {
            auto road_limit_x = 1.0 * std::min(road->GetStart().x, road->GetEnd().x) - 0.4;
            auto move_x_dist = std::max(road_limit_x, max_move.x);

            if(move_x_dist == road_limit_x) {
                dog.Stop();
            }
            return {move_x_dist, dog.GetPos().y};
        }
        case Dir::EAST: {
            auto road_limit_x = 1.0 * std::max(road->GetStart().x, road->GetEnd().x) + 0.4;
            auto move_x_dist = std::min(road_limit_x, max_move.x);

            if(move_x_dist == road_limit_x) {
                dog.Stop();
            }
            return {move_x_dist, dog.GetPos().y};
        }
    }
    return {dog.GetPos().x, dog.GetPos().y};
}

size_t Session::GetDogBagCapacity() const {
    return map_.GetBagCapacity().has_value()
               ? map_.GetBagCapacity().value()
               : settings_.default_bag_capacity;
}

Point2D Session::GetSpawnPoint() const {
    return settings_.randomised_dog_spawn
               ? map_.GetRandomRoadPt()
               : map_.GetFirstRoadPt();
}

void Session::MoveDog(Dog& dog, TimeMs delta_t) {
    auto start = dog.GetPos();
    auto dir = dog.GetDir();

    const auto roadV = map_.FindVertRoad(start);
    const auto roadH = map_.FindHorRoad(start);

    if(!roadH && !roadV) {
        dog.Stop();
        throw std::runtime_error("Dog is not on a road!");
    }

    //move dog for max move or until road limit is hit
    const Road* preferred_road;
    Point2D new_pos;

    if(dir == Dir::NORTH || dir == Dir::SOUTH) {
        //choose vertical road if moving vertically and is available
        preferred_road = roadV ? roadV : roadH;
        new_pos = ComputeDogMove(dog, preferred_road, delta_t);
    } else {
        preferred_road = roadH ? roadH : roadV;
        new_pos = ComputeDogMove(dog, preferred_road, delta_t);
    }

    const auto roadV_check = map_.FindVertRoad(new_pos);
    const auto roadH_check = map_.FindHorRoad(new_pos);

    if(!roadV_check && !roadH_check) {
        dog.Stop();
        throw std::runtime_error("Dog has moved outside of a road!");
    }

    dog.SetPos(new_pos);
}

void Session::MoveAllDogs(TimeMs delta_t) {
    for(auto& [_, dog] : dogs_) {
        MoveDog(dog, delta_t);
    }
}

void Session::ProcessCollisions(TimeMs delta_t) {
    CollisionDetector collision_detector(dogs_, loot_items_, map_.GetOffices(), settings_);
    auto events = collision_detector::FindGatherEvents(collision_detector);

    //Process events in chronological time:
    for(const auto& collision_event : events) {
        auto idx = collision_event.item_id;
        auto& dog = dogs_.at(collision_event.gatherer_id);

        if(collision_detector.IsLootItem(idx)) {
            auto loot_item = loot_items_.at(collision_detector.GetLootIdFromGatherEvent(idx));

            //TODO:check for datarace, mb do this through io/strand
            if(!dog.BagIsFull() && !loot_item.collected) {
                dog.CollectLootItem(loot_item);
            }
        } else if(collision_detector.IsOffice(idx)) {
            for (const auto& item : dog.GetBagItems()) {
                dog.AddScore(item.points);
            }
            dog.UnloadAllItems();
        } else {
            //TODO: Use cerr for now, fix this later
            //throw std::logic_error("Invalid collect event index");
            std::cerr << "-invalid collect event index in collision detector" << std::endl;
        }
    }

    //Remove collected items from map
    //Do this after collision event processing to keep all GatherEvent indexes valid!
    for(auto it = loot_items_.begin(); it != loot_items_.end(); ++it) {
        if(it->second.collected) {
            loot_items_.erase(it);
        }
    }
}

void Session::GenerateLoot(TimeMs delta_t) {
#ifdef GATHER_DEBUG
    if(loot_items_.empty()) {
        AddLootItem(0, {0.0, 50.0});
    }
#else
    auto num_of_new_items = loot_generator_.Generate(delta_t, GetLootCount(), GetDogCount());
    AddRandomLootItems(num_of_new_items);
#endif
}

const Map* Session::GetMap() const {
    return &map_;
}

size_t Session::GetDogCount() const {
    return dogs_.size();
}

size_t Session::GetLootCount() const {
    return loot_items_.size();
}

const Dog* Session::GetDog(const Dog::Id id) {
    const auto dog_it = dogs_.find(id);
    return dog_it == dogs_.end() ? nullptr : &(dog_it->second);
}


//=================================================
//=================== Game ========================
void Game::AddMap(Map map) {
    if(auto [it, inserted] = maps_.emplace(map.GetId(), std::move(map)); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    }
}

void Game::SetRandomDogSpawn(bool enable) {
    settings_.randomised_dog_spawn = enable;
}

Map* Game::FindMap(const Map::Id& id) {
    if(auto it = maps_.find(id); it != maps_.end()) {
        return &(it->second);
    }
    return nullptr;
}

const Map* Game::FindMap(const Map::Id& id) const {
    if(auto it = maps_.find(id); it != maps_.end()) {
        return &(it->second);
    }
    return nullptr;
}

Session* Game::FindSession(const Map::Id& map_id) {
    auto map_ptr = FindMap(map_id);
    if(!map_ptr) {
        throw std::invalid_argument("Incorrect map id, map not found");
    }

    //No session for map
    auto session_it = sessions_.find(map_id);
    if(session_it == sessions_.end()) {
        return nullptr;
    }
    //Session exists
    return &sessions_.at(map_id);
}

Session* Game::MakeSession(const Map::Id& map_id, Session::Strand strand) {
    auto map_ptr = FindMap(map_id);
    if(!map_ptr) {
        throw std::invalid_argument("Incorrect map id, map not found");
    }
    try {
        //TODO: Make session with strand
        auto [sess_it, success] = sessions_.emplace(
            map_id,
            Session{next_session_id_++, *map_ptr, settings_}
        );
        if(!success) {
            throw std::runtime_error("Failed to create Game Session");
        }
        return &(sess_it->second);
    } catch(...) {
        sessions_.erase(map_id);

        //TODO: make sure throw is caught
        std::cerr << "failed to create session" << std::endl;
        //throw std::runtime_error("failed to create session");
        return nullptr;
    }
}

void Game::AdvanceTimeInSessions(TimeMs delta_t) {
    for(auto& [_, session] : sessions_) {
        session.AdvanceTime(delta_t);
    }
}

const Game::Maps& Game::GetMaps() const {
    return maps_;
}

const gamedata::Settings& Game::GetSettings() const {
    return settings_;
}

} //namespace model
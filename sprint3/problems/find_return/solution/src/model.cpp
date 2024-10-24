#include "model.h"

#include <stdexcept>

//DEBUG
#include <iostream>
//#define GATHER_DEBUG

namespace model {
using namespace std::literals;

Point2D::Point2D(Point pt)
    : x(1.0 * pt.x)
    , y(1.0 * pt.y) {
}

Point2D::Point2D(double x, double y)
    : x(x)
    , y(y) {
}

std::ostream& operator<<(std::ostream& os, const Point& pt) {
    os << '(' << pt.x << ", " << pt.y << ')';
    return os;
}

std::ostream& operator<<(std::ostream& os, const Point2D& pt) {
    os << '{' << pt.x << ", " << pt.y << '}';
    return os;
}

std::ostream& operator<<(std::ostream& os, const Speed& pt) {
    os << '[' << pt.Vx << ", " << pt.Vy << ']';
    return os;
}

geom::Point2D to_geom_pt(Point pt) {
    return {1.0 * pt.x, 1.0 * pt.y};
}
geom::Point2D to_geom_pt(Point2D pt) {
    return {pt.x, pt.y};
}

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

void Map::MoveDog(Dog* dog, TimeMs delta_t) const {
    auto start = dog->GetPos();
    auto dir = dog->GetDir();

    const auto roadV = FindVertRoad(start);
    const auto roadH = FindHorRoad(start);

    if(!roadH && !roadV) {
        dog->Stop();
        throw std::runtime_error("Dog is not on a road!");
    }

    //move dog for max move or until road limit is hit
    const Road* preferred_road;
    Point2D new_pos;

    if(dir == Dir::NORTH || dir == Dir::SOUTH) {
        //choose vertical road if moving vertically and is available
        preferred_road = roadV ? roadV : roadH;
        new_pos = ComputeMove(dog, preferred_road, delta_t);
    } else {
        preferred_road = roadH ? roadH : roadV;
        new_pos = ComputeMove(dog, preferred_road, delta_t);
    }

    const auto roadV_check = FindVertRoad(new_pos);
    const auto roadH_check = FindHorRoad(new_pos);

    if(!roadV_check && !roadH_check) {
        dog->Stop();
        throw std::runtime_error("Dog has moved outside of a road!");
    }

    dog->SetPos(new_pos);
}

Point2D Map::ComputeMove(Dog* dog, const Road* road, TimeMs delta_t) const {
    //Maximum point dog can reach in delta_t if no road limit is hit
    auto max_move = dog->ComputeMaxMove(delta_t);
    switch(dog->GetDir()) {
        case Dir::NORTH: {
            auto road_limit_y = 1.0 * std::min(road->GetStart().y, road->GetEnd().y) - 0.4;

            //Choose the least value (closest to starting point, it is the limit)
            auto move_y_dist = std::max(road_limit_y, max_move.y);

            if(move_y_dist == road_limit_y) {
                dog->Stop();
            }
            return {dog->GetPos().x, move_y_dist};
        }
        case Dir::SOUTH: {
            auto road_limit_y = 1.0 * std::max(road->GetStart().y, road->GetEnd().y) + 0.4;
            auto move_y_dist = std::min(road_limit_y, max_move.y);

            if(move_y_dist == road_limit_y) {
                dog->Stop();
            }
            return {dog->GetPos().x, move_y_dist};
        }
        case Dir::WEST: {
            auto road_limit_x = 1.0 * std::min(road->GetStart().x, road->GetEnd().x) - 0.4;
            auto move_x_dist = std::max(road_limit_x, max_move.x);

            if(move_x_dist == road_limit_x) {
                dog->Stop();
            }
            return {move_x_dist, dog->GetPos().y};
        }
        case Dir::EAST: {
            auto road_limit_x = 1.0 * std::max(road->GetStart().x, road->GetEnd().x) + 0.4;
            auto move_x_dist = std::min(road_limit_x, max_move.x);

            if(move_x_dist == road_limit_x) {
                dog->Stop();
            }
            return {move_x_dist, dog->GetPos().y};
        }
    }
    return {dog->GetPos().x, dog->GetPos().y};
}

LootType Map::GetRandomLootTypeNum() const {
    auto rand_type = GenRandomNum(loot_types_->Size());
    assert(rand_type >= 0 && rand_type < loot_types_->Size());
    return static_cast<LootType>(rand_type);
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
    : lbl_(id, std::move(name))
    , pos_(position)
    , bag_capacity_(bag_capacity) {
}

std::string_view Dog::GetName() const {
    return lbl_.name_tag;
}

size_t Dog::GetId() const {
    return lbl_.id;
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

Dog::Label Dog::GetLabel() const {
    return lbl_;
}

Dog& Dog::SetPos(Point2D pos) {
    prev_pos_ = pos_;
    pos_ = pos;
    return *this;
}

Dog& Dog::SetWidth(double width) {
    width_ = width;
    return *this;
}

Dog& Dog::SetBagCap(size_t capacity) {
    bag_capacity_ = capacity;
    return *this;
}

Point2D Dog::ComputeMaxMove(TimeMs delta_t) const {
    //converts to seconds
    double delta_t_sec = std::chrono::duration<double>(delta_t).count();
    return {pos_.x + delta_t_sec * speed_.Vx, pos_.y + delta_t_sec * speed_.Vy};
}

double Dog::GetWidth() const {
    return width_;
}

bool Dog::CollectLootItem(LootItem* loot_ptr) {
    if(!BagIsFull()) {
        bag_.push_back(loot_ptr);
        loot_ptr->collected = true;
        return true;
    }
    return false;
}

void Dog::UnloadAllItems() {
    bag_.clear();
}

bool Dog::BagIsFull() const {
    return bag_.size() >= bag_capacity_;
}

const BagItems& Dog::GetBagItems() const {
    return bag_;
}

//=================================================
//=================== Session =====================
Session::Session(size_t id, Map* map_ptr, const Game* game)
    : id_(id)
    , game_(game)
    , map_(map_ptr)
    , loot_generator_(game_->GetLootGenerator())
    , randomize_dog_spawn_(game_->IsDogSpawnRandom()) {
}

Dog* Session::AddDog(std::string name) {
    auto starting_pos = randomize_dog_spawn_ ? map_->GetRandomRoadPt() : map_->GetFirstRoadPt();
    auto bag_capacity = map_->GetBagCapacity().has_value() ? map_->GetBagCapacity().value() : game_->GetDefaultBagCapacity();

    return &dogs_.emplace_back(next_dog_id_++, std::move(name), starting_pos, bag_capacity);
}

LootItem* Session::AddLootItem(LootType type, Point2D pos) {
    LootItem* loot_ptr = &loot_items_.emplace_back(
        next_loot_id_, type, pos
    );
    loot_item_index_[next_loot_id_++] = loot_ptr;

    return loot_ptr;
}

void Session::AddRandomLootItems(size_t num_items) {
    for(int i = 0; i < num_items; ++i) {
        AddLootItem(
            map_->GetRandomLootTypeNum(),
            map_->GetRandomRoadPt()
        );
    }
}

size_t Session::GetId() const {
    return id_;
}

const Map::Id& Session::GetMapId() const {
    return map_->GetId();
}

const std::deque<Dog>& Session::GetAllDogs() const {
    return dogs_;
}

const std::deque<LootItem>& Session::GetLootItems() const {
    return loot_items_;
}

void Session::AdvanceTime(TimeMs delta_t) {
    GenerateLoot(delta_t);
    MoveAllDogs(delta_t);
    ProcessCollisions(delta_t);
}

void Session::MoveAllDogs(TimeMs delta_t) {
    for(auto& dog : dogs_) {
        map_->MoveDog(&dog, delta_t);
    }
}

void Session::ProcessCollisions(TimeMs delta_t) {
    auto events = collision_detector::FindGatherEvents(*this);
    //TODO: Process collision events in app?

    //Process events in chronological time:
    for(const GatherEvent& collision_event : events) {
        auto idx = collision_event.item_id;
        auto& dog = dogs_.at(collision_event.gatherer_id);

        if(IsLootItem(idx)) {
            //TODO: fix constness? to get rid of cast
            LootItem* loot_item = const_cast<LootItem*>(&GetLootFromGatherEvent(idx));
            if(!loot_item->collected) {
                dog.CollectLootItem(loot_item);
            }
        } else if(IsOffice(idx)) {
            for (const auto& item : dog.GetBagItems()) {
                player_scores_[dog.GetId()] += map_->GetLootItemValue(item->type);
            }
            dog.UnloadAllItems();
        } else {
            throw std::logic_error("Invalid collect event index");
        }
    }

    //Remove collected items from map
    //Do this after collision event processing to keep all GatherEvent indexes valid!
    for(auto it = loot_items_.begin(); it != loot_items_.end(); ++it) {
        if(it->collected) {
            auto id = it->id;
            loot_item_index_.erase(id);
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
    auto num_of_new_items = loot_generator_->Generate(delta_t, GetLootCount(), GetDogCount());
    AddRandomLootItems(num_of_new_items);
#endif
}

const Map* Session::GetMap() const {
    return map_;
}

size_t Session::GetDogCount() const {
    return dogs_.size();
}

size_t Session::GetLootCount() const {
    return loot_items_.size();
}

bool Session::IsLootItem(size_t collision_event_id) const {
    return collision_event_id < loot_items_.size();
}

bool Session::IsOffice(size_t collision_event_id) const {
    return collision_event_id - loot_items_.size() < map_->GetOffices().size();
}

const LootItem& Session::GetLootFromGatherEvent(size_t collision_event_id) const {
    return loot_items_.at(collision_event_id);
}

const Office& Session::GetOfficeFromGatherEvent(size_t collision_event_id) const {
    return map_->GetOffices().at(collision_event_id - loot_items_.size());
}

size_t Session::ItemsCount() const {
    return loot_items_.size() + map_->GetOffices().size();
}

Session::Item Session::GetItem(size_t idx) const {
    //Case 1: LootItem Item
    if(IsLootItem(idx)) {
        auto& item = GetLootFromGatherEvent(idx);
        return {to_geom_pt(item.pos), item.width};
    }
        //Case 2: Office
    else if(IsOffice(idx)) {
        //office vector indexes starting after loot items indexes
        auto& office = GetOfficeFromGatherEvent(idx);
        return {to_geom_pt(office.GetPosition()), office.GetWidth()};
    } else {
        throw std::logic_error("invalid collision event id");
    }
}

size_t Session::GatherersCount() const {
    return dogs_.size();
}

Session::Gatherer Session::GetGatherer(size_t idx) const {
    auto& dog = dogs_.at(idx);
    return {to_geom_pt(dog.GetPrevPos()), to_geom_pt(dog.GetPos()), dog.GetWidth()};
}

size_t Session::GetPlayerScore(size_t dog_id) const {
    return player_scores_.count(dog_id) > 0
    ? player_scores_.at(dog_id)
    : 0;
}

//=================================================
//=================== Game ========================
void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if(auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch(...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

Map* Game::FindMap(const Map::Id& id) {
    auto it = map_id_to_index_.find(id);

    if(auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

const Map* Game::FindMap(const Map::Id& id) const {
    auto it = map_id_to_index_.find(id);

    if(auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

Session* Game::JoinSession(const Map::Id& id) {
    //if session exists and is not yet full
    //TODO: sessions by map, check session full
    auto map_ptr = FindMap(id);
    if(!map_ptr) {
        return nullptr;
    }

    //No session for map
    auto session_it = map_to_sessions_.find(id);
    if(session_it == map_to_sessions_.end()) {
        try {
            //NB: using default loot generator for every session here!
            return &sessions_.emplace_back(std::move(MakeNewSessionOnMap(map_ptr)));
        } catch(...) {
            map_to_sessions_.erase(id);
            throw;
        }
    }
    //Session exists
    return &sessions_.at(session_it->second);
}

std::optional<size_t> Game::GetMapIndex(const Map::Id& id) const {
    if(auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return it->second;
    }
    return std::nullopt;
}

Session Game::MakeNewSessionOnMap(Map* map) {
    map_to_sessions_[map->GetId()] = next_session_id_;
    return {next_session_id_++, map, this};
}

const Game::Maps& Game::GetMaps() const {
    return maps_;
}

void Game::ModifyDefaultDogSpeed(double speed) {
    default_dog_speed_ = speed;
}

void Game::ModifyDefaultBagCapacity(size_t capacity) {
    default_bag_capacity_ = capacity;
}

Game::Sessions& Game::GetSessions() {
    return sessions_;
}

LootGenPtr Game::ConfigLootGenerator(TimeMs base_interval, double probability) {
    loot_generator_ = std::make_shared<loot_gen::LootGenerator>(base_interval, probability);
    return loot_generator_;
}

double Game::GetDefaultDogSpeed() const {
    return default_dog_speed_;
}

size_t Game::GetDefaultBagCapacity() const {
    return default_bag_capacity_;
}
LootGenPtr Game::GetLootGenerator() const {
    return loot_generator_;
}
bool Game::IsDogSpawnRandom() const {
    return random_dog_spawn_;
}
void Game::EnableRandomDogSpawn(bool enable) {
    //is disabled by default
    random_dog_spawn_ = enable;
}
} //namespace model
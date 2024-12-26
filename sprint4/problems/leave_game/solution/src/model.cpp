#include "model.h"

#include <stdexcept>

//DEBUG
#include <iostream>
//#define GATHER_DEBUG

namespace model {
using namespace std::literals;

Distance operator*(const Speed & v, TimeMs t) {
    double time_sec = std::chrono::duration<double>(t).count();
    return {v.x * time_sec, v.y * time_sec};
}

Point2D operator+(Point2D pt_dbl, Distance dist_int) {
    return {pt_dbl.x + 1.0 * dist_int.x, pt_dbl.y + 1.0 * dist_int.y};
}

Point to_int_pt(Point2D pt_double) {
    return Point(std::round(pt_double.x), std::round(pt_double.y));
}

//==== Random int generator for use in game model
int GenRandomNum(int limit1, int limit2) {
    ///'static' allows to reuse generator in the same thread, since expensive to init
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
        return {static_cast<Coord>(rand_x), start_.y};
    }
    auto rand_y = GenRandomNum(start_.y, end_.y);
    return {start_.x, static_cast<Coord>(rand_y)};
    //return {static_cast<Coord>(start_.x, util::random_num(start_.y, end_.y))};
}

Coord Road::GetMaxCoordX() const {
    return IsHorizontal() ? std::max(start_.x, end_.x) : start_.x;
}

Coord Road::GetMinCoordX() const {
    return IsHorizontal() ? std::min(start_.x, end_.x) : start_.x;
}

Coord Road::GetMaxCoordY() const {
    return IsVertical() ? std::max(start_.y, end_.y) : start_.y;
}

Coord Road::GetMinCoordY() const {
    return IsVertical() ? std::min(start_.y, end_.y) : start_.y;
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
//=================== Office ======================
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

//=================================================
//=================== Objects ======================
CollisionObject::CollisionObject(GameObject::Id id, Point2D pos, double width)
    : GameObject(id)
    , pos_(pos)
    , prev_pos_(pos)
    , width_(width) {}

Point2D CollisionObject::GetPos() const {
    return pos_;
}

Point2D CollisionObject::GetPrevPos() const {
    return prev_pos_;
}

CollisionObject& CollisionObject::SetPos(Point2D new_pos) {
    prev_pos_ = pos_;
    pos_ = new_pos;
    return *this;
}

double CollisionObject::GetWidth() const {
    return width_;
}

collision_detector::Item CollisionObject::AsCollisionItem() const {
    return {GetPos(), GetWidth()};
}

bool CollisionObject::IsCollectible() const {
    return false;
}

bool CollisionObject::IsItemsReturn() const {
    return false;
}

LootItemInfo CollisionObject::Collect() {
    return {};
}

Speed DynamicObject::GetSpeed() const {
    return speed_;
}

DynamicObject& DynamicObject::SetSpeed(Speed speed) {
    speed_ = speed;
    return *this;
}

Direction DynamicObject::GetDirection() const {
    return direction_;
}

DynamicObject& DynamicObject::SetDirection(Direction dir) {
    direction_ = dir;
    return *this;
}

DynamicObject& DynamicObject::SetMovement(Direction dir, double speed_value) {
    //remove this condition to get a cool effect of rotating dogs when arrow key is not pressed
    if(dir != Direction::NONE) {
        SetDirection(dir);
    }

    switch(dir) {
        case Direction::NORTH:
            SetSpeed({0.0, -speed_value});
            break;
        case Direction::WEST:
            SetSpeed({-speed_value, 0.0});
            break;
        case Direction::SOUTH:
            SetSpeed({0, speed_value});
            break;
        case Direction::EAST:
            SetSpeed({speed_value, 0.0});
            break;
        case Direction::NONE:
            Stop();
            break;

        default:SetSpeed({0.0, 0.0});
            break;
    }
    return *this;
}

void DynamicObject::Stop() {
    SetSpeed({0.0, 0.0});
}

// Point2D DynamicObject::ComputeMoveEndPoint(TimeMs delta_t) const {
//     //converts to seconds
//     double delta_t_sec = std::chrono::duration<double>(delta_t).count();
//     return {GetPos().x + delta_t_sec * speed_.x, GetPos().y + delta_t_sec * speed_.y};
// }

collision_detector::Gatherer DynamicObject::AsGatherer() const {
    return {GetPrevPos(), GetPos(), GetWidth()};
}

LootItem::LootItem(GameObject::Id id, Point2D pos, double width, LootItem::Type type, Score value)
    : CollisionObject(id, pos, width)
    , type_(type)
    , value_(value) {
}

LootItem::Type LootItem::GetType() const {
    return type_;
}

Score LootItem::GetValue() const {
    return value_;
}

bool LootItem::IsCollectible() const {
    return true;
}

LootItemInfo LootItem::Collect() {
    const bool can_collect_item = !is_collected_;

    //If item is not yet collected, update status
    if(can_collect_item) {
        is_collected_ = true;
    }

    return {GetId(), type_, value_, can_collect_item};
}


ItemsReturnPoint::ItemsReturnPoint(GameObject::Id id, const Office& office, double width)
: CollisionObject(id, ToGeomPt(office.GetPosition()), width)
, tag_(office.GetId()) {
}

bool ItemsReturnPoint::IsItemsReturn() const {
    return true;
}


//=================================================
//=================== Dog =========================
// Dog::Dog(Id id, Point pos, double width, Tag tag, size_t bag_cap)
//     : DynamicObject(id, ToGeomPt(pos), width)
//     , tag_(std::move(tag))
//     , bag_capacity_(bag_cap) {
// }

Dog::Dog(Id id, Point2D pos, double width, std::string name, size_t bag_cap)
    : DynamicObject(id, pos, width)
    , name_(std::move(name))
    , bag_capacity_(bag_cap) {
}

std::string Dog::GetName() const {
    return name_;
}

Dog& Dog::SetBagCap(size_t capacity) {
    bag_capacity_ = capacity;
    return *this;
}

//For restoring game state
bool Dog::TryCollectItem(LootItemInfo loot_info) {
    if(BagIsFull() || !loot_info.can_collect) {
        return false;
    }
    bag_.push_back(std::move(loot_info));
    return true;
}

void Dog::ProcessCollision(const CollisionObjectPtr& obj) {
    //Only two options for now, so use this method:
    // 1.Is a loot item
    if(obj->IsCollectible()) {
        TryCollectItem(obj->Collect());
    }
    // 2.Is an office
    else if(obj->IsItemsReturn()) {
        for(const auto& item : GetBag()) {
            AddScore(item.value);
        }
        ClearBag();
    }
    else {
        throw std::logic_error("unknown collision object type");
    }

    /// If mechanics become more complex, can potentially use pointer_cast
    // spBase base = std::make_shared<Derived>();
    // spDerived derived = std::dynamic_pointer_cast<spDerived::element_type>(base);
}

Dog& Dog::SetMovement(Direction dir, double speed_value) {
    move_cmd_received_ = (dir != Direction::NONE);

    if(move_cmd_received_) {
        is_stopped_ = false;
        ResetInactiveTime();
    }
    //SetMovement calls Stop if dir == none
    DynamicObject::SetMovement(dir, speed_value);
    return *this;
}

Dog& Dog::SetPos(Point2D new_pos) {
    //If dog moves during this tick, reset inactivity time
    if(new_pos != pos_) {
        ResetInactiveTime();
    } else {
        is_stopped_ = true;
    }
    CollisionObject::SetPos(new_pos);
    return *this;
}

bool Dog::IsStopped() const {
    return is_stopped_;
    //return speed_ == Speed{0.0, 0.0};
}

bool Dog::IsExpired() const {
    return !move_cmd_received_ && IsStopped() && inactive_time_ >= max_inactive_time_;
}

void Dog::ClearBag() {
    bag_.clear();
}

bool Dog::BagIsFull() const {
    return bag_.size() >= bag_capacity_;
}

const Dog::BagContent& Dog::GetBag() const {
    return bag_;
}

Dog& Dog::SetRetireTime(TimeMs time_msec) {
    max_inactive_time_ = time_msec;
    return *this;
}

void Dog::AddTime(TimeMs delta_t) {
    ingame_time_ += delta_t;
    //Only add tick time, when the dog is stationary and move cmd was not received
    if(!move_cmd_received_ && IsStopped()) {
        inactive_time_ += delta_t;

        //when dog expires, decrease ingame time by the extra tick time after expiry
        // if(IsExpired()) {
        //     auto tick_time_diff = inactive_time_ - max_inactive_time_;
        //     ingame_time_ -= tick_time_diff;
        // }
    }
    /** NB: This function must be called LAST during tick,
     *  because it depends on player cmd being already processed,
     *  resets bool move_cmd_received */
    move_cmd_received_ = false;
}

size_t Dog::GetBagCap() const {
    return bag_capacity_;
}

Score Dog::GetScore() const {
    return score_;
}

void Dog::RestoreTimers(TimeMs total_time, TimeMs inactive_time) {
    ingame_time_   = total_time;
    inactive_time_ = inactive_time;
}

TimeMs Dog::GetIngameTime() const {
    return ingame_time_;
}

TimeMs Dog::GetInactiveTime() const {
    return inactive_time_;
}

void Dog::AddScore(Score points) {
    score_ += points;
}


//=================================================
//================ Map ============================
Map::Map(Map::Id id, std::string name)
    : id_(std::move(id))
    , name_(std::move(name)) {
}

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
        throw std::runtime_error("Failed to add office to map");;
    }
}

Point2D Map::GetRandomRoadPt() const {
    auto& random_road = roads_.at(GenRandomNum(roads_.size()));
    return ToGeomPt(random_road.GetRandomPt());
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

const Road* Map::FindVertRoad(Point2D point) const {
    auto it = RoadXtoIndex_.find(static_cast<Coord>(std::round(point.x)));
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
        const auto road = &roads_.at(road_idx);
        //skip Horizontal roads
        if(road->IsVertical() && pt_is_on_road(road, point)) {
            return &roads_.at(road_idx);
        }
    }
    return nullptr;
}

const Road* Map::FindHorRoad(Point2D point) const {
    auto pt_is_on_road = [&](const Road* rd, const Point2D& pt) {
        Point check{static_cast<Coord>(std::round(pt.x)), static_cast<Coord>(std::round(pt.y))};
        return (check.x >= std::min(rd->GetStart().x, rd->GetEnd().x))
            && (std::max(rd->GetStart().x, rd->GetEnd().x) >= check.x);
    };

    auto it = RoadYtoIndex_.find(static_cast<Coord>(std::round(point.y)));
    if(it == RoadYtoIndex_.end()) {
        return nullptr;
    }

    for(const auto road_idx : it->second) {
        const auto road = &roads_[road_idx];

        //skip vertical roads
        if(road->IsHorizontal() && pt_is_on_road(road, point)) {
            return &roads_[road_idx];
        }
    }
    return nullptr;
}

Map::MoveResult Map::ComputeRoadMove(Point2D start, Point2D end) const {
    //TODO: on map Town dog sometimes skips to a different road, debug this
    //Case 0: no move
    if(start == end) {
        return {false, end};
    }

    const auto roadV = FindVertRoad(start);
    const auto roadH = FindHorRoad(start);

    //Case 1: start point is not on road
    if(!roadH && !roadV) {
        std::cerr << "DEBUG: Dog is not on a road" << std::endl;
        return {true, start};
    }

    //move for max dist or until road limit is hit
    const Road* preferred_road;
    Point2D move_pos = end;
    double road_limit, move_coord;

    //Cases 2 & 3: Move vertically (Y coord changes)
    if(start.x == end.x) {
        //choose vertical road if available
        preferred_road = roadV ? roadV : roadH;

        if(start.y < end.y) {
            //Case 2: move in Increasing Y
            road_limit = 1.0 * preferred_road->GetMaxCoordY() + 0.4;
            move_coord = std::min(road_limit, end.y);
        } else {
            //Case 3: move in Decreasing Y
            road_limit = 1.0 * preferred_road->GetMinCoordY() - 0.4;
            move_coord = std::max(road_limit, end.y);
        }
        move_pos.y = move_coord;
    } else {
        //Cases 4 & 5: Move in X coord -> prefer horizontal road
        preferred_road = roadH ? roadH : roadV;

        if(start.x < end.x) {
            //Case 4: move in Increasing X
            road_limit = 1.0 * preferred_road->GetMaxCoordX() + 0.4;
            move_coord = std::min(road_limit, end.x);
        } else {
            //Case 5: move in Decreasing X
            road_limit = 1.0 * preferred_road->GetMinCoordX() - 0.4;
            move_coord = std::max(road_limit, end.x);
        }
        move_pos.x = move_coord;
    }

    return {move_coord == road_limit, move_pos};
}

std::optional<size_t> Map::GetBagCapacity() const {
    return bag_capacity_;
}

void Map::SetBagCapacity(size_t cap) {
    bag_capacity_ = cap;
}

Point2D Map::GetFirstRoadPt() const {
    return ToGeomPt(roads_.at(0).GetStart());
}

Score Map::GetLootItemValue(LootType type) const {
    return loot_types_->GetItemValue(type);
}

size_t Map::GetLootTypesSize() const {
    return loot_types_->Size();
}

const gamedata::LootTypesInfo& Map::GetLootTypesInfo() const {
    return *loot_types_;
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

MapPtr Game::FindMap(const Map::Id& id) {
    if(const auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

ConstMapPtr Game::FindMap(const Map::Id& id) const {
    if(const auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

const Game::Maps& Game::GetMaps() const {
    return maps_;
}

const gamedata::Settings& Game::GetSettings() const {
    return settings_;
}

void Game::ModifyDefaultDogSpeed(double speed) {
    settings_.default_dog_speed = speed;
}

void Game::ModifyDefaultBagCapacity(size_t capacity) {
    settings_.default_bag_capacity = capacity;
}

void Game::ModifyDefaultDogTimeout(TimeMs timeout) {
    settings_.default_inactivity_timeout_ms_ = timeout;
}

void Game::EnableRandomDogSpawn(bool enable) {
    //is disabled by default
    settings_.randomised_dog_spawn = enable;
}

void Game::ConfigLootGen(TimeMs base_period, double probability) {
    settings_.loot_gen_interval = base_period;
    settings_.loot_gen_prob = probability;
}

} //namespace model
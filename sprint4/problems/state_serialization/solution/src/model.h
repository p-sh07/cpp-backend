#pragma once
#include <chrono>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/json.hpp>

#include "app_util.h"
#include "collision_detector.h"
#include "game_data.h"
#include "geom.h"

namespace model {
//==== Time, Coord, Geom ==========//
using TimeMs = std::chrono::milliseconds;

using Dimension = int;
using Coord = Dimension;

struct Point {
    Point() = default;

    Point(Coord x, Coord y)
        : x(x)
          , y(y) {
    }

    Coord x = 0, y = 0;
};

using geom::Point2D;
using Distance = geom::Point2D;
using Speed = geom::Vec2D;

enum class Direction : char {
    //Progresses clockwise
    NORTH = 'U',
    EAST = 'R',
    SOUTH = 'D',
    WEST = 'L',
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

using LootType = gamedata::LootItemType;
using Score = size_t;

//==== model::Point <-> geom::Point2D
inline Point2D ToGeomPt(Point pt) {
    return {pt.x * 1.0, pt.y * 1.0};
}

inline Point ToIntPt(Point2D pt_double) {
    return Point(std::round(pt_double.x), std::round(pt_double.y));
}

//==== Op overloads ==========//
Point2D operator+(Point2D pos, Distance dist);

Distance operator*(const Speed& v, TimeMs t);


//==== Random int generator for use in game model ==========//
int GenRandomNum(int limit1, int limit2 = 0);

size_t GenRandomNum(size_t limit1, size_t limit2 = 0);


//=================================================
//========== Stationary Game(Map) Objects =========
class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x);

    Road(VerticalTag, Point start, Coord end_y);

    bool IsHorizontal() const;

    bool IsVertical() const;

    Point GetStart() const;

    Point GetEnd() const;

    Point GetRandomPt() const;

    Coord GetMaxCoordX() const;

    Coord GetMinCoordX() const;

    Coord GetMaxCoordY() const;

    Coord GetMinCoordY() const;

private:
    Point start_;
    Point end_;
};

//------------------------------------------------
class Building {
public:
    explicit Building(Rectangle bounds);

    const Rectangle &GetBounds() const;

private:
    Rectangle bounds_;
};

//------------------------------------------------
class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset);

    const Id &GetId() const;

    Point GetPosition() const;

    Offset GetOffset() const;

private:
    Id id_;
    Point position_;
    Offset offset_;
};


//=================================================
//========== Temporary Game Objects ===============
class GameObject {
public:
    using Id = size_t;

    GameObject(Id id)
        : id_(id) {
    }

    Id GetId() const {
        return id_;
    }

private:
    const Id id_;
};

using GameObjectPtr = std::shared_ptr<GameObject>;
using ConstGameObject = std::shared_ptr<const GameObject>;

//------------------------------------------------
class CollisionObject : public GameObject {
public:
    CollisionObject(Id id, Point2D pos, double width);

    Point2D GetPos() const;

    Point2D GetPrevPos() const;

    CollisionObject &SetPos(Point2D pos);

    double GetWidth() const;

    collision_detector::Item AsCollisionItem() const;

private:
    Point2D pos_;
    Point2D prev_pos_;
    const double width_;
};

using CollisionObjectPtr = std::shared_ptr<CollisionObject>;
using ConstCollisionObjectPtr = std::shared_ptr<const CollisionObject>;

//------------------------------------------------
class DynamicObject : public CollisionObject {
public:
    using CollisionObject::CollisionObject;

    Speed GetSpeed() const;

    DynamicObject &SetSpeed(Speed speed);

    Direction GetDirection() const;

    DynamicObject &SetDirection(Direction dir);

    DynamicObject &SetMovement(Direction dir, double speed_value);

    void Stop();

    Point2D ComputeMoveEndPoint(TimeMs delta_t) const;

    collision_detector::Gatherer AsGatherer() const;

    //TODO: How to do this?
    //virtual void ProcessCollision(CollisionObjectPtr obj) = 0;

private:
    Speed speed_;
    Direction direction_ = Direction::NORTH;
};

using DynamicObjectPtr = std::shared_ptr<DynamicObject>;
using ConstDynamicObjectPtr = std::shared_ptr<const DynamicObject>;

//------------------------------------------------
class LootItem : public CollisionObject {
public:
    using Type = gamedata::LootItemType;

    struct Info {
        Id id;
        Type type;
    };

    LootItem(Id id, Point2D pos, double width, Type type);

    Type GetType() const;

    bool IsCollected() const;

    Info Collect();

private:
    const Type type_;
    bool is_collected_ = false;
};

using LootItemPtr = std::shared_ptr<LootItem>;
using ConstLootItemPtr = std::shared_ptr<const LootItem>;

//------------------------------------------------
class ItemsReturnPoint : public CollisionObject {
public:
    ItemsReturnPoint(GameObject::Id id, const Office& office, double width);

private:
    const Office::Id tag_;
};

using ItemsReturnPointPtr = std::shared_ptr<ItemsReturnPoint>;
using ConstItemsReturnPointPtr = std::shared_ptr<const ItemsReturnPoint>;


//=================================================
//=================== Dog =========================
class Dog : public DynamicObject {
public:
    using Tag = util::Tagged<std::string, Dog>;
    using BagContent = std::deque<LootItem::Info>;

    Dog(Id id, Point pos, double width, Tag tag, size_t bag_cap);

    Dog(Id id, Point2D pos, double width, Tag tag, size_t bag_cap);

    Tag GetTag() const;

    size_t GetBagCap() const;

    Dog &SetBagCap(size_t capacity);

    const BagContent &GetBag() const;

    void AddScore(Score points);

    Score GetScore() const;

    bool BagIsFull() const;

    void ClearBag();

    bool TryCollectItem(const LootItemPtr& loot);

    bool TryCollectItem(const LootItem::Info& loot_info);

private:
    //what is a dog? Upd: A dog is a dynamic collision object
    const Tag tag_;
    size_t bag_capacity_{0u};
    Score score_{0u};

    BagContent bag_;
};

using DogPtr = std::shared_ptr<Dog>;
using ConstDogPtr = std::shared_ptr<const Dog>;


//=================================================
//=================== Map =========================
class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    struct MoveResult {
        bool road_edge_reached_;
        Point2D dst;
    };

    Map(Id tag, std::string name);

    const Id &GetId() const;

    const std::string &GetName() const;

    const Buildings &GetBuildings() const;

    const Roads &GetRoads() const;

    const Offices &GetOffices() const;

    const gamedata::LootTypesInfo &GetLootTypesInfo() const;

    size_t GetLootTypesSize() const;

    Score GetLootItemValue(LootType type) const;

    std::optional<double> GetDogSpeed() const;

    void SetDogSpeed(double speed);

    std::optional<size_t> GetBagCapacity() const;

    void SetBagCapacity(size_t cap);

    void AddRoad(const Road& road);

    void AddBuilding(const Building& building);

    void AddOffice(Office office);

    template<typename LootTypeData>
    void AddLootInfo(LootTypeData&& loot_types) {
        loot_types_ = std::make_unique<gamedata::LootTypesInfo>(std::forward<LootTypeData>(loot_types));
    }

    const Road* FindVertRoad(Point2D point) const;

    const Road* FindHorRoad(Point2D point) const;

    MoveResult ComputeRoadMove(Point2D start, Point2D end) const;

    Point GetRandomRoadPt() const;

    Point GetFirstRoadPt() const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    std::optional<double> dog_speed_;
    std::optional<size_t> bag_capacity_;

    Offices offices_;
    OfficeIdToIndex warehouse_id_to_index_;

    using RoadIndex = std::unordered_map<Coord, std::unordered_set<size_t>>;
    RoadIndex RoadXtoIndex_;
    RoadIndex RoadYtoIndex_;

    std::unique_ptr<gamedata::LootTypesInfo> loot_types_ = nullptr;
};

using MapPtr = Map*;
using ConstMapPtr = const Map*;


//NB: Default values set here!
struct GameSettings {
    bool randomise_dog_spawn = false;

    std::optional<double> map_dog_speed;
    std::optional<size_t> map_bag_capacity;

    double loot_gen_prob;
    TimeMs loot_gen_interval;

    double default_dog_speed = 1.0;
    size_t default_bag_capacity = 3;

    const double loot_item_width = 0.0;
    const double dog_width = 0.6;
    const double office_width = 0.5;

    double GetDogSpeed() const;

    double GetBagCap() const;
};


class Game {
public:
    using Maps = std::vector<Map>;
    using MapIdHasher = util::TaggedHasher<Map::Id>;

    void AddMap(Map map);

    void EnableRandomDogSpawn(bool enable);

    void ModifyDefaultDogSpeed(double speed);

    void ModifyDefaultBagCapacity(size_t capacity);

    void ConfigLootGen(TimeMs base_period, double probability);

    const Maps &GetMaps() const;

    GameSettings GetSettings() const;

    MapPtr FindMap(const Map::Id& id);

    ConstMapPtr FindMap(const Map::Id& id) const;

private:
    GameSettings settings_; //TODO: = gamedata::default_game_settings?;
    size_t next_session_id_ = 0;

    Maps maps_;

    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    MapIdToIndex map_id_to_index_;
};

template<typename LootContainer, typename OfficeContainer, typename DogContainer>
class ItemEventHandler final : collision_detector::ItemGathererProvider {
    using Item = collision_detector::Item;
    using Gatherer = collision_detector::Gatherer;
    using Event = collision_detector::GatheringEvent;

public:
    struct EventResult {
        GameObject::Id obj_id;
        GameObject::Id dog_id;
        bool is_loot_collect = false;
        bool is_items_return = false;
    };

    ItemEventHandler(const LootContainer& loot_items, const OfficeContainer& offices, const DogContainer& gatherers)
        : loot_items_(loot_items)
          , offices_(offices)
          , gatherers_(gatherers) {
    }

    std::vector<EventResult> FindCollisions() const {
        std::vector<EventResult> result;
        auto collision_events = collision_detector::FindGatherEvents(*this);
        for (const Event& event : collision_events) {
            GameObject::Id obj_id = 0;
            bool is_loot_item = IsLootItem(event.item_id);
            if (is_loot_item) {
                obj_id = event.item_id;
            } else {
                obj_id = GetOfficeIdx(event.item_id);
            }
            result.emplace_back(obj_id, event.gatherer_id, is_loot_item, is_loot_item);
        }
        return result;
    }

    size_t ItemsCount() const override {
        return loot_items_.size() + offices_.size();
    }

    collision_detector::Item GetItem(size_t idx) const override {
        if (IsLootItem(idx)) {
            return loot_items_.at(idx)->AsCollisionItem();
        }
        return offices_.at(GetOfficeIdx(idx))->AsCollisionItem();
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx)->AsGatherer();
    }

private:
    const LootContainer& loot_items_;
    const OfficeContainer& offices_;
    const DogContainer& gatherers_;

    bool IsLootItem(size_t idx) const {
        return idx < loot_items_.size();
    }

    bool IsOffice(size_t idx) const {
        if (IsLootItem()) {
            return false;
        }
        return GetOfficeIdx(idx) < offices_.size();
    }

    size_t GetOfficeIdx(size_t idx) const {
        if (IsLootItem(idx)) {
            throw std::out_of_range("");
        }
        return idx - loot_items_.size();
    }
};
} // namespace model

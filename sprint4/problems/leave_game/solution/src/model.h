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
    NONE = 'N',
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

    const Rectangle& GetBounds() const;

private:
    Rectangle bounds_;
};

//------------------------------------------------
class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset);

    const Id& GetId() const;

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
struct LootItemInfo {
    LootItemInfo() = default;
    LootItemInfo(GameObject::Id id, LootType type)
        : id(id)
        , type(type) {
    }

    LootItemInfo(GameObject::Id id, LootType type, Score value, bool is_collectible)
        : id(id)
        , type(type)
        , value(value)
        , can_collect(is_collectible){
    }

    GameObject::Id id {0u};
    LootType type {0u};
    Score value {0u};
    bool can_collect {true};

    auto operator<=>(const LootItemInfo&) const = default;
};

//------------------------------------------------
class CollisionObject : public GameObject {
public:
    using PtrToOther = const std::shared_ptr<CollisionObject>;

    CollisionObject(Id id, Point2D pos, double width);
    virtual ~CollisionObject() = default;

    Point2D GetPos() const;
    Point2D GetPrevPos() const;

    //Redefined for dog to allow inactivity tracking
    virtual CollisionObject& SetPos(Point2D new_pos);

    double GetWidth() const;
    collision_detector::Item AsCollisionItem() const;

    virtual bool IsCollectible() const;
    virtual bool IsItemsReturn() const;
    virtual LootItemInfo Collect();

protected:
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
    DynamicObject& SetSpeed(Speed speed);

    Direction GetDirection() const;
    DynamicObject& SetDirection(Direction dir);

    //This function needs to be redefined for dogs to allow inactive time keeping
    virtual DynamicObject& SetMovement(Direction dir, double speed_value);
    void Stop();

    collision_detector::Gatherer AsGatherer() const;

protected:
    Speed speed_;
    Direction direction_ = Direction::NORTH;
};

using DynamicObjectPtr = std::shared_ptr<DynamicObject>;
using ConstDynamicObjectPtr = std::shared_ptr<const DynamicObject>;

//------------------------------------------------
class LootItem : public CollisionObject {
public:
    using Type = gamedata::LootItemType;
    LootItem(Id id, Point2D pos, double width, Type type, Score value);

    Type GetType() const;
    Score GetValue() const;

    bool IsCollectible() const override;

    LootItemInfo Collect() override;

private:
    const Type type_;
    const Score value_;
    bool is_collected_ = false;
};

using LootItemPtr = std::shared_ptr<LootItem>;
using ConstLootItemPtr = std::shared_ptr<const LootItem>;

//------------------------------------------------
class ItemsReturnPoint : public CollisionObject {
public:
    ItemsReturnPoint(Id id, const Office& office, double width);

    bool IsItemsReturn() const override;

private:
    const Office::Id tag_;
};

using ItemsReturnPointPtr = std::shared_ptr<ItemsReturnPoint>;
using ConstItemsReturnPointPtr = std::shared_ptr<const ItemsReturnPoint>;


//=================================================
//=================== Dog =========================
class Dog : public DynamicObject {
public:
    using BagContent = std::deque<LootItemInfo>;

    // Dog(Id id, Point pos, double width, Tag tag, size_t bag_cap);
    Dog(Id id, Point2D pos, double width, std::string name, size_t bag_cap);

    std::string GetName() const;
    size_t GetBagCap() const;
    Dog& SetBagCap(size_t capacity);
    const BagContent& GetBag() const;
    Dog& SetRetireTime(TimeMs time_msec);

    void AddTime(TimeMs delta_t);
    void RestoreTimers(TimeMs total_time, TimeMs inactive_time);
    TimeMs GetIngameTime() const;
    TimeMs GetInactiveTime() const;

    void AddScore(Score points);
    Score GetScore() const;

    bool BagIsFull() const;
    void ClearBag();

    bool TryCollectItem(LootItemInfo loot_info);
    void ProcessCollision(const CollisionObjectPtr& obj);

    Dog& SetMovement(Direction dir, double speed_value) override;
    Dog& SetPos(Point2D new_pos) override;
    bool IsStopped() const;
    bool IsExpired() const;

private:
    //what is a dog? Upd: A dog is a dynamic collision object
    const std::string name_;
    size_t bag_capacity_{0u};
    Score score_{0u};

    //Dogs expire only on the next tick after they have become inactive
    bool move_cmd_received_ {false};

    TimeMs ingame_time_{0u};
    TimeMs inactive_time_{0u};

    //default max inactive time is 60 seconds
    TimeMs max_inactive_time_{60000u};

    BagContent bag_;

    //Time is reset if movement End Point is different from Starting Point
    void ResetInactiveTime() {
        inactive_time_ = TimeMs{0u};
    }
};

using DogPtr = Dog*;
using ConstDogPtr = const Dog*;


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

    Map(Id id, std::string name);

    const Id& GetId() const;
    const std::string& GetName() const;
    const Buildings& GetBuildings() const;
    const Roads& GetRoads() const;
    const Offices& GetOffices() const;
    const gamedata::LootTypesInfo& GetLootTypesInfo() const;
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

    Point2D GetRandomRoadPt() const;
    Point2D GetFirstRoadPt() const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    std::optional<double> dog_speed_;
    std::optional<size_t> bag_capacity_;
    std::optional<TimeMs> dog_timeout_;

    Offices offices_;
    OfficeIdToIndex warehouse_id_to_index_;

    using RoadIndex = std::unordered_map<Coord, std::unordered_set<size_t>>;
    RoadIndex RoadXtoIndex_;
    RoadIndex RoadYtoIndex_;

    std::unique_ptr<gamedata::LootTypesInfo> loot_types_ = nullptr;
};

using MapPtr = Map*;
using ConstMapPtr = const Map*;


class Game {
public:
    using Maps = std::vector<Map>;
    using MapIdHasher = util::TaggedHasher<Map::Id>;

    void AddMap(Map map);

    void EnableRandomDogSpawn(bool enable);
    void ModifyDefaultDogSpeed(double speed);
    void ModifyDefaultBagCapacity(size_t capacity);
    void ModifyDefaultDogTimeout(TimeMs timeout);

    void ConfigLootGen(TimeMs base_period, double probability);

    const Maps& GetMaps() const;
    const gamedata::Settings& GetSettings() const;

    MapPtr FindMap(const Map::Id& id);
    ConstMapPtr FindMap(const Map::Id& id) const;

private:
    gamedata::Settings settings_;
    size_t next_session_id_ = 0;

    Maps maps_;

    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    MapIdToIndex map_id_to_index_;
};

template<typename ObjPtrContainer, typename DogPtrContainer>
class CollisionDetector final : collision_detector::ItemGathererProvider {
    using Item = collision_detector::Item;
    using Gatherer = collision_detector::Gatherer;

public:
    using Event = collision_detector::GatheringEvent;

    CollisionDetector(const ObjPtrContainer& objects, const DogPtrContainer& gatherers)
        : objects_(objects)
        , gatherers_(gatherers) {
    }

    std::vector<Event> FindCollisions() const {
        return collision_detector::FindGatherEvents(*this);
    }

    size_t ItemsCount() const override {
        return objects_.size();
    }

    collision_detector::Item GetItem(size_t idx) const override {
        return objects_.at(idx)->AsCollisionItem();
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx)->AsGatherer();
    }

private:
    const ObjPtrContainer& objects_;
    const DogPtrContainer& gatherers_;
};
} // namespace model

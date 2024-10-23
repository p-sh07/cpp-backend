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
#include "geom.h"
#include "game_data.h"
#include "loot_generator.h"

namespace model {
//== NB: Default params
static constexpr double DEFAULT_ITEM_WIDTH = 0.0;
static constexpr double DEFAULT_OFFICE_WIDTH = 0.5;
static constexpr double DEFAULT_PLAYER_WIDTH = 0.6;

//== Time & Coord params
using TimeMs = std::chrono::milliseconds;

using Dimension = int;
using Coord = Dimension;

using Dimension2D = double;
using Coord2D = Dimension2D;

struct Point {
    Coord x, y;
};

struct Point2D {
    Point2D() = default;
    Point2D(double x, double y);
    Point2D(Point pt);

    auto operator<=>(const Point2D&) const = default;

    Coord2D x, y;
};

struct Speed {
    Dimension2D Vx, Vy;
};

//== Op overloads
std::ostream& operator<<(std::ostream& os, const Point& pt);
std::ostream& operator<<(std::ostream& os, const Point2D& pt);
std::ostream& operator<<(std::ostream& os, const Speed& pt);

//== Transforms for collision detections
geom::Point2D to_geom_pt(Point pt);
geom::Point2D to_geom_pt(Point2D pt);

//==== Random int generator for use in game model
int GenRandomNum(int limit1, int limit2 = 0);
size_t GenRandomNum(size_t limit1, size_t limit2 = 0);

//Progresses clockwise
enum class Dir : char {
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

//TODO: refactor with inheritance id,getid etc - common base class?
//TODO: Change tagged_id from id_ to tag_
//class GameObject {
//    GameObject() = default;
//};

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

    Road(HorizontalTag, Point start, Coord end_x)  ;
    Road(VerticalTag, Point start, Coord end_y)  ;

    inline bool IsHorizontal() const;
    inline bool IsVertical() const;
    inline Point GetStart() const;
    Point GetEnd() const;
    Point GetRandomPt() const;

 private:
    Point start_;
    Point end_;
};

class Building {
 public:
    explicit Building(Rectangle bounds)  ;
    const Rectangle& GetBounds() const;

 private:
    Rectangle bounds_;
};

class Office {
 public:
    using Id = util::Tagged<std::string, Office>;
    Office(Id id, Point position, Offset offset)  ;

    const Id& GetId() const;
    Point GetPosition() const;
    Offset GetOffset() const;
    double GetWidth() const;

 private:
    Id id_;
    Point position_;
    Offset offset_;
    double width_ = DEFAULT_OFFICE_WIDTH;
};

using LootType = unsigned;

struct LootItem {
    size_t id;
    LootType type;
    Point2D pos;
    double width = DEFAULT_ITEM_WIDTH;
    bool collected = false;
};

using BagItems = std::deque<LootItem*>;

class Dog {
 public:
    Dog(size_t id, std::string name, Point2D position, size_t bag_capacity);

    // Get
    std::string_view GetName() const;
    size_t GetId() const;
    Point2D GetPos() const;
    Point2D GetPrevPos() const;
    Speed GetSpeed() const;
    Dir GetDir() const;
    double GetWidth() const;
    bool BagIsFull() const;
    const BagItems& GetBagItems() const;

    // Set
    Dog& SetPos(Point2D pos);
    Dog& SetSpeed(Speed sp);
    Dog& SetWidth(double width);
    Dog& SetBagCap(size_t capacity);
    Dog& SetDir(Dir dir);
    Dog& SetMove(Dir dir, double speed_value);
    Dog& Stop();

    //Move dog for delta t = 10 ms => moves 0.1 m at speed 1;
    //TODO: Assuming speed = 1 unit / sec
    Point2D ComputeMaxMove(TimeMs delta_t) const;

    // Item gathering
    bool CollectLootItem(LootItem* loot_ptr);
    //std::vector<LootType>
    void UnloadAllItems();

    //TODO: currently unused:
    struct Label {
        size_t id;
        std::string name_tag;
    };
    //Use Label to allow dogs with same name and fast search in map by name+id
    Label GetLabel() const;

 private:
    //what is a dog?
    const Label lbl_;
    Point2D pos_ = {0.0, 0.0};

    Speed speed_ = {0, 0};
    Dir direction_ = Dir::NORTH;

    //for item gathering
    size_t bag_capacity_ = 0;
    BagItems bag_;
    Point2D prev_pos_ = {0.0, 0.0};
    double width_ = DEFAULT_PLAYER_WIDTH;
};

class Map {
 public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;


    Map(Id id, std::string name);

    const Id& GetId() const;
    const std::string& GetName() const;

    const Buildings& GetBuildings() const;
    const Roads& GetRoads() const;
    const Offices& GetOffices() const;

    const size_t GetLootItemValue(LootType type) const {
        return loot_types_->GetItemValue(type);
    }
    const gamedata::LootTypeInfo* GetLootInfo() const;

    std::optional<double> GetDogSpeed() const;
    void SetDogSpeed(double speed);
    std::optional<size_t> GetBagCapacity() const;
    void SetBagCapacity(size_t cap);

    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);

    template<typename LootTypeData>
    void AddLootInfo(LootTypeData&& loot_types) {
        //NB: using forward here, check in case of obscure compile errors!
        loot_types_ = std::make_unique<gamedata::LootTypeInfo>(std::forward<LootTypeData>(loot_types));
    }

    const Road* FindVertRoad(Point2D pt) const;
    const Road* FindHorRoad(Point2D pt) const;

    Point GetRandomRoadPt() const;

    Point GetFirstRoadPt() const {
        return roads_.at(0).GetStart();
    }

    void MoveDog(Dog* dog, TimeMs delta_t) const;
    LootType GetRandomLootTypeNum() const;

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
    std::unordered_map<Coord, std::unordered_set<size_t>> RoadXtoIndex_;
    std::unordered_map<Coord, std::unordered_set<size_t>> RoadYtoIndex_;

    Point2D ComputeMove(Dog* dog, const Road* road, TimeMs delta_t) const;
    std::unique_ptr<gamedata::LootTypeInfo> loot_types_ = nullptr;
};

using LootGenPtr = std::shared_ptr<loot_gen::LootGenerator>;
class Game;

class Session : collision_detector::ItemGathererProvider {
    using LootIdToItem = std::unordered_map<size_t, LootItem*>;
    using Item = collision_detector::Item;
    using Gatherer = collision_detector::Gatherer;
    using GatherEvent = collision_detector::GatheringEvent;

 public:
    Session(size_t id, Map* map, const Game* game);

    size_t GetId() const;
    const Map::Id& GetMapId() const;
    const Map* GetMap() const;
    size_t GetDogCount() const;
    size_t GetLootCount() const;
    size_t GetPlayerScore(size_t dog_id) const;

    const std::deque<Dog>& GetAllDogs() const;
    const std::deque<LootItem>& GetLootItems() const;

    //At construction there are 0 dogs. Session is always on 1 map
    //When a player is added, he gets a new dog to control
    Dog* AddDog(std::string name);
    LootItem* AddLootItem(LootType type, Point2D pos);
    void AddRandomLootItems(size_t num_items);

    void AdvanceTime(TimeMs delta_t);

    //Item gathering:
    size_t ItemsCount() const override;
    Item GetItem(size_t idx) const override;
    size_t GatherersCount() const override;
    Gatherer GetGatherer(size_t idx) const override;

 private:
    const size_t id_;
    Map* map_;
    const Game* game_;
    TimeMs time_;
    bool randomize_dog_spawn_ = false;
    LootGenPtr loot_generator_ = nullptr;

    size_t next_dog_id_ = 0;
    size_t next_loot_id_ = 0;

    std::deque<Dog> dogs_;
    std::unordered_map<size_t, size_t> player_scores_;
    std::deque<LootItem> loot_items_;
    LootIdToItem loot_item_index_;

    void MoveAllDogs(TimeMs delta_t);
    void GenerateLoot(TimeMs delta_t);
    void ProcessCollisions(TimeMs delta_t);

    bool IsLootItem(size_t collision_event_id) const;
    bool IsOffice(size_t collision_event_id) const;
    const LootItem& GetLootFromGatherEvent(size_t collision_event_id) const;
    const Office& GetOfficeFromGatherEvent(size_t collision_event_id) const;
};

using MapIdHasher = util::TaggedHasher<Map::Id>;

class Game {
 public:
    using Maps = std::vector<Map>;
    using Sessions = std::deque<Session>;

    void AddMap(Map map);
    void EnableRandomDogSpawn(bool enable);
    void ModifyDefaultDogSpeed(double speed);
    void ModifyDefaultBagCapacity(size_t capacity);

    const Maps& GetMaps() const;
    double GetDefaultDogSpeed() const;
    size_t GetDefaultBagCapacity() const;

    LootGenPtr GetLootGenerator() const;
    bool IsDogSpawnRandom() const;

    Map* FindMap(const Map::Id& id);
    const Map* FindMap(const Map::Id& id) const;

    //std::shared_ptr<Session> AddSession(const Map::Id& id);
    //TODO: Move sessions out of model to application
    Session* JoinSession(const Map::Id& id);
    Sessions& GetSessions();

    LootGenPtr ConfigLootGenerator(TimeMs base_interval, double probability);

 private:
    bool random_dog_spawn_ = false;
    size_t next_session_id_ = 0;

    //NB: Default values set here!
    double default_dog_speed_ = 1.0;
    size_t default_bag_capacity_ = 3;

    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapIdToSessions = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::optional<size_t> GetMapIndex(const Map::Id& id) const;

    Maps maps_;
    Sessions sessions_;

    LootGenPtr loot_generator_ = nullptr;

    MapIdToSessions map_to_sessions_;
    MapIdToIndex map_id_to_index_;

    Session MakeNewSessionOnMap(Map* map);
};

} // namespace model

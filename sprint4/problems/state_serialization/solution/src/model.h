#pragma once
#include <chrono>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

//To enable strand for session
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/json.hpp>

#include "util.h"
#include "collision_detector.h"
#include "geom.h"
#include "game_data.h"
#include "loot_generator.h"

namespace model {
namespace net = boost::asio;

///== Default params
//TODO: move to Gamesettings
static constexpr double DEFAULT_ITEM_WIDTH = 0.0;
static constexpr double DEFAULT_OFFICE_WIDTH = 0.5;
static constexpr double DEFAULT_PLAYER_WIDTH = 0.6;

//==== Random int generator for use in game model
int GenRandomNum(int limit1, int limit2 = 0);
size_t GenRandomNum(size_t limit1, size_t limit2 = 0);

//==== Geom Params (common) ====
using geom::Dimension;
using geom::Coord;
using geom::Point;
using geom::Size;
using geom::Offset;
using geom::Rectangle;
using geom::Dimension2D;
using geom::Coord2D;
using geom::Point2D;
using geom::Vec2D;
using geom::Speed;

//==== Dir ========
enum class Dir : char {
    NORTH = 'U',
    EAST = 'R',
    SOUTH = 'D',
    WEST = 'L',
};

//== Time & other params ======
using TimeMs = std::chrono::milliseconds;
using Score = size_t;

//====================== Game objects =====================
//TODO: use tagged size_t for id's, store objects as unordered_map-> id == shared_ptr ? or just unordered_map id->obj

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
    explicit Building(Rectangle bounds);
    const Rectangle& GetBounds() const;

 private:
    Rectangle bounds_;
};

class Office {
 public:
    using Id = util::Tagged<std::string, Office>;
    Office(Id id, Point position, Offset offset);

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

struct LootItem {
    using Id = size_t;
    using Type = unsigned;

    Id id;
    Type type;
    Score points;

    Point2D pos;
    bool collected = false;
};

struct LootItemInfo {
    LootItem::Id id;
    LootItem::Type type;
    Score points;

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

    size_t GetLootItemValue(LootItem::Type type) const;
    const gamedata::LootTypeInfo* GetLootInfo() const;

    std::optional<double> GetDogSpeed() const;
    void SetDogSpeed(double speed);

    std::optional<size_t> GetBagCapacity() const;
    void SetBagCapacity(size_t cap);

    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);

    template<typename LootTypeData>
    void AddLootInfo(LootTypeData&& loot_types);

    const Road* FindVertRoad(Point2D pt) const;
    const Road* FindHorRoad(Point2D pt) const;

    Point GetRandomRoadPt() const;
    Point GetFirstRoadPt() const;
    LootItem::Type GetRandomLootTypeNum() const;

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
    std::unique_ptr<gamedata::LootTypeInfo> loot_types_ = nullptr;
};

template<typename LootTypeData>
void Map::AddLootInfo(LootTypeData&& loot_types) {
    //NB: using forward here, check in case of obscure compile errors!
    loot_types_ = std::make_unique<gamedata::LootTypeInfo>(std::forward<LootTypeData>(loot_types));
}


class Dog {
 public:
    using Id = size_t;
    using Name = std::string;
    using Bag = std::deque<LootItemInfo>;

    Dog(Id id, Name name, Point2D position, size_t bag_capacity);

    // Get
    std::string_view GetName() const;
    size_t GetId() const;
    Point2D GetPos() const;
    Point2D GetPrevPos() const;
    Speed GetSpeed() const;
    Dir GetDir() const;
    Score GetScore() const;
    const Bag& GetBagItems() const;

    bool BagIsFull() const;

    // Set
    Dog& SetPos(Point2D pos);
    Dog& SetSpeed(Speed sp);
    Dog& SetBagCap(size_t capacity);
    Dog& SetDir(Dir dir);
    Dog& SetMove(Dir dir, double speed_value);
    Dog& AddScore(Score points);;
    Dog& SetScore(Score total);
    Dog& Stop();

    //Move dog for delta t = 10 ms => moves 0.1 m at speed 1;
    //TODO: remove this function?
    Point2D ComputeMaxMove(TimeMs delta_t) const;

    // Item gathering
    bool CollectLootItem(LootItem& item);
    void UnloadAllItems();

 private:
    //what is a dog?
    Id id_            {0u};
    std::string name_ {};
    Point2D pos_      {0.0, 0.0};
    Point2D prev_pos_ {0.0, 0.0};
    Speed speed_      {0, 0};
    Dir direction_    {Dir::NORTH};

    //for item gathering
    Bag bag_;
    size_t bag_capacity_ = 0u;
    size_t score_        = 0u;
};

//TODO: implement to use dogs, items and offices?
template<typename DogContainer, typename LootContainer, typename OfficesContainer>
class CollisionDetector : public collision_detector::ItemGathererProvider {
    using Item = collision_detector::Item;
    using Gatherer = collision_detector::Gatherer;
    using GatherEvent = collision_detector::GatheringEvent;
public:
    CollisionDetector(const DogContainer& dogs, const LootContainer& loot_items, const OfficesContainer& offices, const gamedata::Settings& game_settings)
        : dogs_(dogs)
        , loot_(loot_items)
        , offices_(offices)
        , settings_(game_settings) {

        UpdateObjectIndices();

        }

    //Item gathering:
    size_t ItemsCount() const override {
        return loot_.size() + offices_.size();
    }

    Item GetItem(size_t idx) const override {
        //Case 1: LootItem Item
        if(IsLootItem(idx)) {
            auto& item = loot_.at(GetLootIdFromGatherEvent(idx));
            return {item.pos, settings_.loot_item_width};
        }
        //Case 2: Office
        else if(IsOffice(idx)) {
            //office vector indexes starting after loot items indexes
            auto& office = GetOfficeFromGatherEvent(idx);
            return {office.GetPosition(), settings_.office_width};
        } else {
            throw std::logic_error("invalid collision event id");
        }
    }

    size_t GatherersCount() const override {
        return dogs_.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        auto& dog = dogs_.at(dog_index_.at(idx));
        return {dog.GetPrevPos(), dog.GetPos(), settings_.dog_width};
    }

    bool IsLootItem(size_t collision_event_id) const {
        return collision_event_id < loot_index_.size();
    }

    bool IsOffice(size_t collision_event_id) const {
        return collision_event_id - loot_index_.size() < offices_.size();
    }

    LootItem::Id GetLootIdFromGatherEvent(size_t collision_event_id) const {
        //TODO: ensure throw from here gets caught
        return loot_index_.at(collision_event_id);
    }

    const Office& GetOfficeFromGatherEvent(size_t collision_event_id) const {
        return offices_.at(collision_event_id - loot_index_.size());
    }

private:
    std::vector<Dog::Id> dog_index_;
    std::vector<LootItem::Id> loot_index_;

    const DogContainer& dogs_;
    const LootContainer& loot_;
    const OfficesContainer& offices_;

    //NB: use settings for items/gatherers widths
    const gamedata::Settings settings_;

    void UpdateObjectIndices() {
        dog_index_.reserve(dogs_.size());
        for(const auto& [dog_id, _] : dogs_) {
            dog_index_.push_back(dog_id);
        }

        loot_index_.reserve(loot_.size());
        for(const auto& [loot_id, _] : loot_) {
            loot_index_.push_back(loot_id);
        }
    }

};

//==== Session manages Dogs, Loot, Movement and Collisions, Scores ====
//NB: Session is completely independent of the Game. Game just defines parameters through settings
//TODO: Map ref or (shared)ptr is better?
class Session {
 public:
    using Id = size_t;
    using Strand = net::strand<net::io_context::executor_type>;
    using Dogs = std::unordered_map<Dog::Id, Dog>;
    using LootItems = std::unordered_map<LootItem::Id, LootItem>;

    Session(Id id, const Map& map, gamedata::Settings settings);

    //TODO: Add strand
    //Session(Id id, const Map& map, gamedata::Settings settings, Strand strand);

    size_t GetId() const;
    const Map::Id& GetMapId() const;
    const Map* GetMap() const;
    const Dog* GetDog(Dog::Id id);

    size_t GetDogCount() const;
    size_t GetLootCount() const;

    const Dogs& GetAllDogs() const;
    const LootItems& GetLootItems() const;

    //Complete control over params for restoring sessions from save
    Dog* AddDog(Dog::Id id, Dog::Name name, Point2D pos);
    LootItem* AddLootItem(LootItem::Id id, LootItem::Type type, Point2D pos);

    //Id's and pos processed internally
    Dog* AddDog(Dog::Name name);
    LootItem* AddLootItem(LootItem::Type type);

    void AddRandomLootItems(size_t num_items);
    void AdvanceTime(TimeMs delta_t);

 private:
    const Id id_;
    const Map& map_;
    gamedata::Settings settings_;
    loot_gen::LootGenerator loot_generator_;
    // CollisionDetector<Dogs, LootItems, Map::Offices> collision_detector_;

    Dog::Id next_dog_id_ = 0u;
    LootItem::Id next_loot_id_ = 0u;

    Dogs dogs_;
    LootItems loot_items_;

    size_t GetDogBagCapacity() const;
    Point2D GetSpawnPoint() const;

    static Point2D ComputeDogMove(Dog& dog, const Road* road, TimeMs delta_t);
    void MoveDog(Dog& dog, TimeMs delta_t);
    void MoveAllDogs(TimeMs delta_t);

    void GenerateLoot(TimeMs delta_t);
    void ProcessCollisions(TimeMs delta_t);
};


//=== Game contains maps and Settings, manages Sessions on Maps
class Game {
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapSessions = std::unordered_map<Map::Id, Session, MapIdHasher>;

public:
    using Maps = std::unordered_map<Map::Id, Map, MapIdHasher>;

    explicit Game(gamedata::Settings game_settings)
        : settings_(std::move(game_settings)) {
    }

    void AddMap(Map map);
    void SetRandomDogSpawn(bool enable);

    const Maps& GetMaps() const;
    const gamedata::Settings& GetSettings() const;

    Map* FindMap(const Map::Id& id);
    const Map* FindMap(const Map::Id& id) const;

    Session* FindSession(const Map::Id& map_id);
    Session* MakeSession(const Map::Id& map_id, Session::Strand strand);
    void AdvanceTimeInSessions(TimeMs delta_t);

private:
    Session::Id next_session_id_ = 0u;
    gamedata::Settings settings_{};
    Maps maps_;
    MapSessions sessions_;
};

} // namespace model

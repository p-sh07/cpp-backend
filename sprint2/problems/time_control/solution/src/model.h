#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>

#include <boost/json.hpp>

#include "app_util.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

using DimensionDbl = double;
using CoordDbl = DimensionDbl;

using Time = double;

struct Point {
    Coord x, y;
};

struct PointDbl {
    PointDbl() = default;
    PointDbl(double x, double y);
    PointDbl(Point pt);

    CoordDbl x, y;
};

struct Speed {
    DimensionDbl vx, vy;
};

//Progresses clockwise
enum class Dir : char {
    NORTH = 'U',
    EAST = 'L',
    SOUTH = 'D',
    WEST = 'R',
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

 private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Dog {
 public:
    struct Label {
        size_t id;
        std::string name_tag;
    };

    Dog(size_t id, std::string name, PointDbl pos);

    std::string_view GetName() const;
    size_t GetId() const;

    PointDbl GetPos() const;
    Speed GetSpeed() const;
    Dir GetDir() const;

    void Stop();

    //Move dog for delta t = 10 ms => moves 0.1 m at speed 1;
    //TODO: Assuming speed = 1 unit / sec; !
    PointDbl ComputeMove(double delta_t) const;

    void SetPos(PointDbl pos);
    void SetSpeed(Speed sp);
    void SetDir(Dir dir);

    //Set speed according to direction dir and speed value s_val (get from map settings)
    void SetMove(Dir dir, double s_val);

    //Use Label to allow dogs with same name and fast search in map by name+id
    Label GetLabel() const;

 private:
    //what is a dog?
    const Label lbl_;
    PointDbl pos_;
    Speed speed_ {0,0};
    Dir direction_ {Dir::NORTH};
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

    double GetDogSpeed() const;
    void SetDogSpeed(double speed);

    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);

    const Road* FindVertRoad(PointDbl pt) const;
    const Road* FindHorRoad(PointDbl pt) const;

    Point GetRandomRoadPt() const;

    Point GetFirstRoadPt() const {
        return roads_.at(0).GetStart();
    }

    void MoveDog(Dog* dog, Time delta_t) const;

 private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    double dog_speed_ {0.0};

    Offices offices_;
    OfficeIdToIndex warehouse_id_to_index_;
    std::unordered_map<Coord, size_t> RoadXtoIndex_;
    std::unordered_map<Coord, size_t> RoadYtoIndex_;

    PointDbl ComputeMaxMove(const Dog* dog, const Road* road, Time delta_t) const;
};

class Session {
 public:
    Session(size_t id, Map* map)  ;

    size_t GetId() const;
    const Map::Id& GetMapId() const;
    const Map* GetMap() const {
        return map_;
    }

    const std::deque<Dog>& GetAllDogs() const;

    //At construction there are 0 dogs. Session is always on 1 map
    //When a player is added, he gets a new dog to control
    Dog* AddDog(std::string name);
    void AdvanceTime(Time delta_t);

 private:
    const size_t id_;
    Map* map_;
    Time time_ = 0;

    size_t next_dog_id_ = 0;
    std::deque<Dog> dogs_;

    void MoveAllDogs(Time delta_t);

};

using MapIdHasher = util::TaggedHasher<Map::Id>;

class Game {
 public:
    using Maps = std::vector<Map>;
    using Sessions = std::deque<Session>;

    void AddMap(Map map);
    void SetDefaultDogSpeed(double speed);

    const Maps& GetMaps() const;

    Map* FindMap(const Map::Id& id);
    const Map* FindMap(const Map::Id& id) const;

    //std::shared_ptr<Session> AddSession(const Map::Id& id);
    Session* JoinSession(const Map::Id& id);
    Sessions& GetSessions();

 private:
    size_t next_session_id_{0};
    double default_dog_speed_{1.0};

    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapIdToSessions = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::optional<size_t> GetMapIndex(const Map::Id& id) const;

    Maps maps_;
    Sessions sessions_;

    MapIdToSessions map_to_sessions_;
    MapIdToIndex map_id_to_index_;

    Session MakeNewSessionOnMap(Map* map);

};

//======= Json conversion overloads ===========
namespace json = boost::json;

//Pos, dir, speed
void tag_invoke(const json::value_from_tag&, json::value& jv, Point const& pt);
Point tag_invoke(const json::value_to_tag<Point>&, json::value const& jv);

void tag_invoke(const json::value_from_tag&, json::value& jv, PointDbl const& pt);
PointDbl tag_invoke(const json::value_to_tag<PointDbl>&, json::value const& jv);

void tag_invoke(const json::value_from_tag&, json::value& jv, Dir const& dir);
Dir tag_invoke(const json::value_to_tag<Dir>&, json::value const& jv);

void tag_invoke(const json::value_from_tag&, json::value& jv, Speed const& speed);
Speed tag_invoke(const json::value_to_tag<Speed>&, json::value const& jv);

//Map overloads
void tag_invoke(const json::value_from_tag&, json::value& jv, Map const& map);
Map tag_invoke(const json::value_to_tag<Map>&, json::value const& jv);

//Road
void tag_invoke(const json::value_from_tag&, json::value& jv, Road const& rd);
Road tag_invoke(const json::value_to_tag<Road>&, json::value const& jv);

//Building
void tag_invoke(const json::value_from_tag&, json::value& jv, Building const& bd);
Building tag_invoke(const json::value_to_tag<Building>&, json::value const& jv);

//Office
void tag_invoke(const json::value_from_tag&, json::value& jv, Office const& offc);
Office tag_invoke(const json::value_to_tag<Office>&, json::value const& jv);

} // namespace model

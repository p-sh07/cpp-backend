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

struct Point {
    Coord x, y;
};

struct PointDbl {
    PointDbl() = default;
    explicit PointDbl(double x, double y);
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

class Map {
 public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name)  ;

    const Id& GetId() const;
    const std::string& GetName() const;

    const Buildings& GetBuildings() const;
    const Roads& GetRoads() const;
    const Offices& GetOffices() const;

    bool HasSpeedSet() const;
    double GetDogSpeed() const;
    void SetDogSpeed(double speed);

    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);

    void AddOffice(Office office);
    Point GetRandomRoadPt() const;

 private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    std::optional<double> dog_speed_;

    //Use this value if optional not set
    double dog_speed_default_{1.0};

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
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

    //Use Label to allow dogs with same name and fast search in map by name+id
    Label GetLabel() const;

 private:
    //what is a dog?
    const Label lbl_;
    PointDbl pos_;
    Speed speed_ {0,0};
    Dir direction_ {Dir::NORTH};
};

class Session {
 public:
    Session(size_t id, const Map* map)  ;

    size_t GetId() const;
    const Map::Id& GetMapId() const;;

    const std::deque<Dog>& GetAllDogs() const;

    //At construction there are 0 dogs. Session is always on 1 map
    //When a player is added, he gets a new dog to control
    Dog* AddDog(std::string name);

 private:
    const size_t id_;
    const Map* map_;

    size_t next_dog_id_ = 0;
    std::deque<Dog> dogs_;

};

using MapIdHasher = util::TaggedHasher<Map::Id>;

class Game {
 public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const  ;
    const Map* FindMap(const Map::Id& id) const  ;

    //std::shared_ptr<Session> AddSession(const Map::Id& id);
    Session* JoinSession(const Map::Id& id);

 private:
    size_t next_session_id_ = 0;

    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapIdToSessions = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::optional<size_t> GetMapIndex(const Map::Id& id) const;

    std::vector<Map> maps_;
    std::deque<Session> sessions_;

    MapIdToSessions map_to_sessions_;
    MapIdToIndex map_id_to_index_;

    Session MakeNewSessionOnMap(const Map* map);

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

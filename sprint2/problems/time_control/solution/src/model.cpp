#include "model.h"

#include <stdexcept>

//DEBUG
#include <iostream>

namespace model {
using namespace std::literals;

PointDbl::PointDbl(Point pt)
    : x(1.0 * pt.x)
    , y(1.0 * pt.y) {
}

PointDbl::PointDbl(double x, double y)
    : x(x)
    , y(y) {
}

//=================================================
//=================== Road ========================
Point Road::GetRandomPt() const {
    if(IsHorizontal()) {
        return {static_cast<Coord>(util::random_num(start_.x, end_.x)), start_.y};
    }
    return {static_cast<Coord>(start_.x, util::random_num(start_.y, end_.y))};
}
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
    auto& random_road = roads_[util::random_num(0, roads_.size())];
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

    if(rd.IsVertical()) {
        RoadXtoIndex_[rd.GetStart().x] = roads_.size() - 1;
    } else {
        RoadYtoIndex_[rd.GetStart().y] = roads_.size() - 1;
    }
}

double Map::GetDogSpeed() const {
    return dog_speed_;
}

void Map::SetDogSpeed(double speed) {
    dog_speed_ = speed;
}
const Road* Map::FindVertRoad(PointDbl pt) const {
    auto it = RoadXtoIndex_.find(static_cast<Coord>(pt.x));
    if(it == RoadXtoIndex_.end()) {
        return nullptr;
    }
    return &roads_[it->second];
}
const Road* Map::FindHorRoad(PointDbl pt) const {
    //TODO:static cast or round? road width == 0.4
    auto it = RoadYtoIndex_.find(static_cast<Coord>(pt.y));
    if(it == RoadYtoIndex_.end()) {
        return nullptr;
    }
    return &roads_[it->second];
}
void Map::MoveDog(Dog* dog, Time delta_t) const {
    auto start = dog->GetPos();
    auto dir = dog->GetDir();

    const auto roadV = FindVertRoad(start);
    const auto roadH = FindHorRoad(start);

    if(!roadH && !roadV) {
        throw std::runtime_error("Dog is not on a road!");
    }

    //move dog for max move or until road limit is hit
    const Road* preferred_road;
    PointDbl new_pos;
    if(dir == Dir::NORTH || dir == Dir::SOUTH) {
        //choose vertical road if moving vertically and is available
        preferred_road = roadV ? roadV : roadH;
        new_pos = ComputeMaxMove(dog, preferred_road, delta_t);
    } else {
        preferred_road = roadH ? roadH : roadV;
        new_pos = ComputeMaxMove(dog, preferred_road, delta_t);
    }

    //TODO:remove debug output
    std::cerr << "Moved dog in Dir[" << (static_cast<char>(dog->GetDir())) << "] along road ["
              << preferred_road->GetStart().x << ", " << preferred_road->GetStart().y << "]["
              << preferred_road->GetEnd().x << ", " << preferred_road->GetEnd().y << "] from pt ("
              << dog->GetPos().x << ", " << dog->GetPos().y << ") to pt ("
              << new_pos.x << ", " << new_pos.y << std::endl;

    dog->SetPos(new_pos);
}
PointDbl Map::ComputeMaxMove(const Dog* dog, const Road* road, Time delta_t) const {
    //Maximum point dog can reach in delta_t if no road limit is hit
    auto max_move = dog->ComputeMove(delta_t);
    switch (dog->GetDir()) {
        case Dir::NORTH: {
            auto road_limit_y = 1.0 * std::min(road->GetStart().y, road->GetEnd().y) - 0.4;

            //Choose the least value (closest to starting point, it is the limit)
            return {max_move.x, std::max(road_limit_y, max_move.y)};
        }
        case Dir::SOUTH: {
            auto road_limit_y = 1.0 * std::max(road->GetStart().y, road->GetEnd().y) + 0.4;
            return {max_move.x, std::min(road_limit_y, max_move.y)};
        }
        case Dir::WEST: {
            auto road_limit_x = 1.0 * std::min(road->GetStart().x, road->GetEnd().x) - 0.4;
            return {std::max(road_limit_x, max_move.x), max_move.y};
        }
        case Dir::EAST: {
            auto road_limit_x = 1.0 * std::max(road->GetStart().x, road->GetEnd().x) + 0.4;
            return {std::min(road_limit_x, max_move.x), max_move.y};
        }
    }
    return {};
}

//=================================================
//=================== Dog =========================
Dog::Dog(size_t id, std::string name, PointDbl pos = {0.0, 0.0})
    : lbl_{id, std::move(name)}
    , pos_(pos) {
}

std::string_view Dog::GetName() const {
    return lbl_.name_tag;
}

size_t Dog::GetId() const {
    return lbl_.id;
}

PointDbl Dog::GetPos() const {
    return pos_;
}

Speed Dog::GetSpeed() const {
    return speed_;
}

Dir Dog::GetDir() const {
    return direction_;
}

void Dog::Stop() {
    speed_ = {0, 0};
}
void Dog::SetSpeed(Speed sp) {
    speed_ = sp;
}
void Dog::SetDir(Dir dir) {
    direction_ = dir;
}
void Dog::SetMove(Dir dir, double s_val) {
    direction_ = dir;
    switch(dir) {
        case Dir::NORTH:SetSpeed({0.0, -s_val});
            break;
        case Dir::WEST:SetSpeed({s_val, 0.0});
            break;
        case Dir::SOUTH:SetSpeed({0, s_val});
            break;
        case Dir::EAST:SetSpeed({-s_val, 0.0});
            break;

        default:SetSpeed({0.0, 0.0});
            break;
    }
}

Dog::Label Dog::GetLabel() const {
    return lbl_;
}
void Dog::SetPos(PointDbl pos) {
    pos_ = pos;
}
PointDbl Dog::ComputeMove(double delta_t) const {
    return {pos_.x + delta_t * speed_.vx, pos_.y + delta_t * speed_.vy};
//    switch(direction_) {
//        case Dir::NORTH:
//            return {pos_.x, pos_.y + delta_t * speed_.vy};
//        case Dir::SOUTH:
//            return {pos_.x, pos_.y + delta_t * speed_.vy};
//        case Dir::WEST:
//            return {pos_.x + delta_t * speed_.vx, pos_.y};
//        case Dir::EAST:
//            return {pos_.x + delta_t * speed_.vx, pos_.y};
//        default:
//            return pos_;
//    }
}

//=================================================
//=================== Session =====================
Session::Session(size_t id, Map* map_ptr)
    : id_(id)
    , map_(map_ptr) {
}

Dog* Session::AddDog(std::string name) {
    //TODO: Change from First Point !!!
    return &dogs_.emplace_back(next_dog_id_++, std::move(name), map_->GetFirstRoadPt());
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
void Session::AdvanceTime(Time delta_t) {
    MoveAllDogs(delta_t);
}
void Session::MoveAllDogs(Time delta_t) {
    for(auto& dog : dogs_) {
        map_->MoveDog(&dog, delta_t);
    }
}

//=================================================
//=================== Game ========================
void Game::AddMap(Map map) {
    //If map speed has not been set in json, set to game default
    if(map.GetDogSpeed() == 0.0) {
        map.SetDogSpeed(default_dog_speed_);
    }

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
    return {next_session_id_++, map};
}

const Game::Maps& Game::GetMaps() const {
    return maps_;
}

void Game::SetDefaultDogSpeed(double speed) {
    default_dog_speed_ = speed;
}
Game::Sessions& Game::GetSessions() {
    return sessions_;
}


//===========================================================
//================= Tag Invoke overloads ====================
namespace json = boost::json;

void tag_invoke(const json::value_from_tag&, json::value& jv, Point const& pt) {
    json::array ja;

    ja.emplace_back(static_cast<int>(pt.x));
    ja.emplace_back(static_cast<int>(pt.y));

    jv = std::move(ja);
}
Point tag_invoke(const json::value_to_tag<Point>&, json::value const& jv) {
    const auto& arr = jv.as_array();
    return {value_to<int>(arr.at(0)), value_to<int>(arr.at(1))};
}

void tag_invoke(const json::value_from_tag&, json::value& jv, PointDbl const& pt) {
    json::array ja;
    ja.emplace_back(pt.x);
    ja.emplace_back(pt.y);

    jv = std::move(ja);
}
PointDbl tag_invoke(const json::value_to_tag<PointDbl>&, json::value const& jv) {
    const auto& arr = jv.as_array();
    PointDbl pt(value_to<double>(arr.at(0)), value_to<double>(arr.at(1)));
    return pt;
}

void tag_invoke(const json::value_from_tag&, json::value& jv, Dir const& dir) {
    jv = std::string{static_cast<char>(dir)};
}
Dir tag_invoke(const json::value_to_tag<Dir>&, json::value const& jv) {
    Dir dir(value_to<Dir>(jv.at("dir")));
    return dir;
}

void tag_invoke(const json::value_from_tag&, json::value& jv, Speed const& speed) {
    json::array ja;
    ja.emplace_back(static_cast<double>(speed.vx));
    ja.emplace_back(static_cast<double>(speed.vy));

    jv = std::move(ja);
}
Speed tag_invoke(const json::value_to_tag<Speed>&, json::value const& jv) {
    auto ja = jv.as_array();
    Speed sp{value_to<DimensionDbl>(jv.at(0)), value_to<DimensionDbl>(jv.at(1))};
    return sp;
}

void tag_invoke(const json::value_from_tag&, json::value& jv, Map const& map) {
    jv = {
        {"id", *map.GetId()},
        {"name", map.GetName()},
    };
}
Map tag_invoke(const json::value_to_tag<Map>&, json::value const& jv) {
    Map::Id id(value_to<std::string>(jv.at("id")));
    return {id, value_to<std::string>(jv.at("name"))};
}

void tag_invoke(const json::value_from_tag&, json::value& jv, Road const& rd) {
    if(rd.IsHorizontal()) {
        jv = {
            {"x0", rd.GetStart().x},
            {"y0", rd.GetStart().y},
            {"x1", rd.GetEnd().x}
        };
    } else {
        jv = {
            {"x0", rd.GetStart().x},
            {"y0", rd.GetStart().y},
            {"y1", rd.GetEnd().y}
        };
    }
}
Road tag_invoke(const json::value_to_tag<Road>&, json::value const& jv) {
    auto const& obj = jv.as_object();

    Point start{
        value_to<int>(obj.at("x0")),
        value_to<int>(obj.at("y0"))
    };

    if(obj.contains("x1")) {
        //Horisontal road
        return {Road::HORIZONTAL, start, value_to<int>(obj.at("x1"))};
    } else {
        //Vertical road
        return {Road::VERTICAL, start, value_to<int>(obj.at("y1"))};
    }
}

void tag_invoke(const json::value_from_tag&, json::value& jv, Building const& bd) {
    const auto& rect = bd.GetBounds();
    jv = {
        {"x", rect.position.x},
        {"y", rect.position.y},
        {"w", rect.size.width},
        {"h", rect.size.height},
    };
}
Building tag_invoke(const json::value_to_tag<Building>&, json::value const& jv) {
    const auto& obj = jv.as_object();
    return Building({
                        //Rectangle:
                        Point{value_to<int>(obj.at("x")), value_to<int>(obj.at("y"))},
                        Size{value_to<int>(obj.at("w")), value_to<int>(obj.at("h"))}
                    });
}

void tag_invoke(const json::value_from_tag&, json::value& jv, model::Office const& offc) {
    jv = {
        {"id", *offc.GetId()},
        {"x", offc.GetPosition().x},
        {"y", offc.GetPosition().y},
        {"offsetX", offc.GetOffset().dx},
        {"offsetY", offc.GetOffset().dy},
    };
}
Office tag_invoke(const json::value_to_tag<Office>&, json::value const& jv) {
    const auto& obj = jv.as_object();
    return {
        Office::Id(value_to<std::string>(obj.at("id"))),
        Point{value_to<int>(obj.at("x")), value_to<int>(obj.at("y"))},
        Offset{value_to<int>(obj.at("offsetX")), value_to<int>(obj.at("offsetY"))}
    };
}

}  // namespace model

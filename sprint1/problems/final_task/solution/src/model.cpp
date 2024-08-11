#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

namespace json = boost::json;

    void tag_invoke(const json::value_from_tag &, json::value &jv,
                    Map const &map) {
        jv = {
            {"id", *map.GetId()},
            {"name", map.GetName()},
        };
    }

    Map tag_invoke(const json::value_to_tag<Map> &, json::value const &jv) {
        Map::Id id(value_to<std::string>(jv.at("id")));
        return {id, value_to<std::string>(jv.at("name"))};
    }

    void tag_invoke(const json::value_from_tag &, json::value &jv, Road const &rd) {
        if (rd.IsHorizontal()) {
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

    Road tag_invoke(const json::value_to_tag<Road> &, json::value const &jv) {
        auto const &obj = jv.as_object();

        Point start{
            value_to<int>(obj.at("x0")),
            value_to<int>(obj.at("y0"))
        };

        if (obj.contains("x1")) {
            //Horisontal road
            return {Road::HORIZONTAL, start, value_to<int>(obj.at("x1"))};
        } else {
            //Vertical road
            return {Road::HORIZONTAL, start, value_to<int>(obj.at("y1"))};
        }
    }

    void tag_invoke(const json::value_from_tag &, json::value &jv, Building const &bd) {
        const auto &rect = bd.GetBounds();
        jv = {
            {"x", rect.position.x},
            {"y", rect.position.y},
            {"w", rect.size.width},
            {"h", rect.size.height},
        };
    }

    Building tag_invoke(const json::value_to_tag<Building> &, json::value const &jv) {
        const auto &obj = jv.as_object();
        return Building({
            //Rectangle:
            Point{value_to<int>(obj.at("x")), value_to<int>(obj.at("y"))},
            Size{value_to<int>(obj.at("w")), value_to<int>(obj.at("h"))}
        });
    }

    void tag_invoke(const json::value_from_tag &, json::value &jv,
                    model::Office const &offc) {
        jv = {
            {"id", *offc.GetId()},
            {"x", offc.GetPosition().x},
            {"y", offc.GetPosition().y},
            {"offsetX", offc.GetOffset().dx},
            {"offsetY", offc.GetOffset().dy},
        };
    }

    Office tag_invoke(const json::value_to_tag<Office> &, json::value const &jv) {
        const auto &obj = jv.as_object();
        return {
            Office::Id(value_to<std::string>(obj.at("id"))),
            Point{value_to<int>(obj.at("x")), value_to<int>(obj.at("y"))},
            Offset{value_to<int>(obj.at("offsetX")), value_to<int>(obj.at("offsetY"))}
        };
    }

    json::value MapToValue(const Map &map) {
        return {
            {"id", *map.GetId()},
            {"name", map.GetName()},
            {"roads", json::value_from(map.GetRoads())},
            {"buildings", json::value_from(map.GetBuildings())},
            {"offices", json::value_from(map.GetOffices())},
        };
    }

}  // namespace model

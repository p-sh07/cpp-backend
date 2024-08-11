#include "json_loader.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace json_loader {
    namespace json = boost::json;

    using namespace model;
    using namespace std::literals;

    static constexpr json::string_view MAP_KEY = "maps";
    static constexpr json::string_view ID_KEY = "id";
    static constexpr json::string_view NAME_KEY = "name";
    static constexpr json::string_view ROADS_KEY = "roads";
    static constexpr json::string_view BUILDINGS_KEY = "buildings";
    static constexpr json::string_view OFFICES_KEY = "offices";

    // void tag_invoke(json::value_from_tag, json::value &jv,
    //                              Map const& map) {
    //     jv = {
    //         {"id", *map.GetId()},
    //         {"name", map.GetName()},
    //     };
    // }

    std::string PrintMapList(const Game& game) {
        json::array map_list_js;

        for(const auto& map : game.GetMaps()) {
            json::object map_js;
            map_js.emplace(*map.GetId(), map.GetName());

            map_list_js.push_back(map_js);
        }

        std::stringstream ss;
        ss << json::serialize(map_list_js);
        return ss.str();
    }

    json::object MakeRoadJson(const Road& road) {
        json::object road_js;
        road_js.emplace("x0", road.GetStart().x);
        road_js.emplace("y0", road.GetStart().y);

        if(road.IsHorizontal()) {
            road_js.emplace("x1", road.GetEnd().x);
        } else {
            road_js.emplace("y1", road.GetEnd().y);
        }
        return road_js;
    }

    json::object MakeBuildingJson(const Building& bd) {
        json::object bd_js;
        bd_js.emplace("x", bd.GetBounds().position.x);
        bd_js.emplace("y", bd.GetBounds().position.y);

        bd_js.emplace("w", bd.GetBounds().size.width);
        bd_js.emplace("h", bd.GetBounds().size.height);

        return bd_js;
    }

    json::object MakeOfficeJson(const Office& offc) {
        json::object offc_js;
        offc_js.emplace("id", *offc.GetId());
        offc_js.emplace("x", offc.GetPosition().x);
        offc_js.emplace("y", offc.GetPosition().y);
        offc_js.emplace("offsetX", offc.GetOffset().dx);
        offc_js.emplace("offsetY", offc.GetOffset().dy);

        return offc_js;
    }

    std::string PrintMap(const Map& map) {
        json::object map_js;

        map_js.emplace(ID_KEY, *map.GetId());
        map_js.emplace(NAME_KEY, map.GetName());

        json::array roads_js;
        for(const auto& road : map.GetRoads()) {
            roads_js.push_back(MakeRoadJson(road));
        }

        map_js.emplace(ROADS_KEY, roads_js);

        json::array buildings_js;
        for(const auto& bd : map.GetBuildings()) {
            buildings_js.push_back(MakeBuildingJson(bd));
        }

        map_js.emplace(BUILDINGS_KEY, roads_js);

        json::array offcs_js;
        for(const auto& offc : map.GetOffices()) {
            offcs_js.push_back(MakeOfficeJson(offc));
        }

        map_js.emplace(OFFICES_KEY, offcs_js);

        std::stringstream ss;
        ss << json::serialize(map_js);
        return ss.str();
    }

    Map BuildMapFromJson(const json::value &map_json) {
        const auto &map_json_dict = map_json.as_object();
        const auto id_ptr = map_json_dict.if_contains(ID_KEY);
        const auto name_ptr = map_json_dict.if_contains(NAME_KEY);

        if (!id_ptr || !name_ptr) {
            throw std::logic_error("No id or name key found in map json");
        }

        Map new_map(Map::Id(json::value_to<std::string>(*id_ptr)), json::value_to<std::string>(*name_ptr));

        //Add roads etc:
        const auto buildings_ptr = map_json_dict.if_contains(BUILDINGS_KEY);
        const auto offices_ptr = map_json_dict.if_contains(OFFICES_KEY);

        //TODO: using tag ?
        if (const auto roads_ptr = map_json_dict.if_contains(ROADS_KEY)) {
            for (const auto &road_jv: roads_ptr->as_array()) {
                Point start;
                start.x = static_cast<Coord>(road_jv.as_object().at("x0").as_int64());
                start.y = static_cast<Coord>(road_jv.as_object().at("y0").as_int64());

                //Horizontal road
                if (const auto x_coord = road_jv.as_object().if_contains("x1")) {
                    new_map.AddRoad({Road::HORIZONTAL, start, static_cast<Coord>(x_coord->as_int64())});
                } else {
                    //Vertical road
                    new_map.AddRoad({
                        Road::VERTICAL, start,
                        static_cast<Coord>(road_jv.as_object().at("y1").as_int64())
                    });
                }
            }
        }

        //Buildings
        if (const auto buildings_ptr = map_json_dict.if_contains(BUILDINGS_KEY)) {
            for (const auto &bld_jv: buildings_ptr->as_array()) {
                Point pos;
                pos.x = static_cast<Coord>(bld_jv.as_object().at("x").as_int64());
                pos.y = static_cast<Coord>(bld_jv.as_object().at("y").as_int64());

                Size sz;
                sz.width = static_cast<Dimension>(bld_jv.as_object().at("w").as_int64());
                sz.height = static_cast<Dimension>(bld_jv.as_object().at("h").as_int64());

                new_map.AddBuilding(Building({pos, sz}));
            }
        }

        //Offices
        if (const auto offices_ptr = map_json_dict.if_contains(OFFICES_KEY)) {
            for (const auto &offc_jv: offices_ptr->as_array()) {
                const auto &offc_obj = offc_jv.as_object();
                Office::Id id(std::string(offc_obj.at("id").as_string()));

                Point pos;
                pos.x = static_cast<Coord>(offc_obj.at("x").as_int64());
                pos.y = static_cast<Coord>(offc_obj.at("y").as_int64());

                Offset ofs;
                ofs.dx = static_cast<Dimension>(offc_obj.at("offsetX").as_int64());
                ofs.dy = static_cast<Dimension>(offc_obj.at("offsetY").as_int64());

                new_map.AddOffice({id, pos, ofs});
            }
        }
        return new_map;
    }

    model::Game LoadGame(const std::filesystem::path &json_path) {
        // Загрузить содержимое файла json_path, например, в виде строки
        std::ifstream input_file(json_path);
        std::string input(std::istreambuf_iterator<char>(input_file), {});

        // Распарсить строку как JSON, используя boost::json::parse
        auto doc = json::parse(input);

        const auto map_array_ptr = doc.as_object().if_contains(MAP_KEY);

        if (!map_array_ptr) {
            throw std::logic_error("No maps in file");
        }

        // Загрузить модель игры из файла
        model::Game game;

        //TODO: Map constructor from json?
        for (const auto &json_map: map_array_ptr->as_array()) {
            try {
                game.AddMap(BuildMapFromJson(json_map));
            } catch (std::exception &ex) {
                std::cerr << ex.what() << std::endl;
            }
        }
        return game;
    }
} // namespace json_loader

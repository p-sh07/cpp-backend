#include "json_loader.h"
#include "server_logger.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace json_loader {

namespace logging = boost::log;
using namespace model;
using namespace std::literals;

//https://live.boost.org/doc/libs/1_83_0/libs/json/doc/html/json/examples.html
void print_json(std::ostream& os, json::value const& jv, std::string* indent = nullptr) {
#ifdef JSON_PRETTY_PRINT
    std::string indent_;
    if(!indent)
        indent = &indent_;
    switch(jv.kind()) {
        case json::kind::object: {
            os << "{\n";
            indent->append(2, ' ');
            auto const& obj = jv.get_object();
            if(!obj.empty()) {
                auto it = obj.begin();
                for(;;) {
                    os << *indent << json::serialize(it->key()) << ": ";
                    print_json(os, it->value(), indent);
                    if(++it == obj.end())
                        break;
                    os << ",\n";
                }
            }
            os << "\n";
            indent->resize(indent->size() - 2);
            os << *indent << "}";
            break;
        }

        case json::kind::array: {
            os << "["; //\n";
            //indent->append(4, ' ');
            auto const& arr = jv.get_array();
            if(!arr.empty()) {
                auto it = arr.begin();
                for(;;) {
                    std::string zero_indent;
                    //os << *indent;
                    print_json(os, *it, &zero_indent);
                    if(++it == arr.end())
                        break;
                    os << ", "; //\n";
                }
            }
            //os << "\n";
            //indent->resize(indent->size() - 4);
            //os << *indent <<
            os << "]";
            break;
        }

        case json::kind::string: {
            os << json::serialize(jv.get_string());
            break;
        }

        case json::kind::uint64:os << jv.get_uint64();
            break;

        case json::kind::int64:os << jv.get_int64();
            break;

        case json::kind::double_:os << jv.get_double();
            break;

        case json::kind::bool_:
            if(jv.get_bool())
                os << "true";
            else
                os << "false";
            break;

        case json::kind::null:os << "null";
            break;
    }

//    if(indent->empty())
//        os << "\n";
#else
    os << json::serialize(jv);
#endif
}

std::string PrintMapList(const model::Game::Maps& map_list) {
    json::array map_list_js;

    for(const auto& map : map_list) {
        map_list_js.push_back(json::value_from(map));
    }
    std::stringstream ss;
    print_json(ss, map_list_js);

    return ss.str();
    //return json::serialize(map_list_js);
}

std::string PrintMap(const Map& map) {
    std::stringstream ss;
    print_json(ss, MapToValue(map));

    return ss.str();
}

json::object MakePlayerListJson(const std::vector<app::PlayerPtr>& players) {
    json::object player_list;
    for(const auto& p_ptr : players) {
        player_list.emplace(std::to_string(p_ptr->GetId()),
                            json::object{
                                {"name", *(p_ptr->GetDog()->GetTag())}
                            }
        );
    }
    return player_list;
}

json::object MakePlayerStateJson(const std::vector<app::PlayerPtr>& players) {
    json::object player_state;
    for(const auto& player : players) {
        const auto& dog = player->GetDog();
        player_state.emplace(std::to_string(player->GetId()), json::object{
                                 {"pos", json::value_from(dog->GetPos()).as_array()},
                                 {"speed", json::value_from(dog->GetSpeed()).as_array()},
                                 {"dir", json::value_from(dog->GetDirection())},
                                 {"bag", json::value_from(dog->GetBag()).as_array()},
                                 {"score", json::value_from(player->GetScore())},
                             }
        );
    }
    return player_state;
}

template<typename LootObjContainer>
json::object MakeLostObjectsJson(const LootObjContainer& loot_objects) {
    json::object loot_jobj;
    size_t num = 0;
    for(const auto& item : loot_objects) {

        loot_jobj.emplace(std::to_string(num++), json::object{
                              {"type", item->GetType()},
                              {"pos", json::value_from(item->GetPos()).as_array()}
                          }
        );
    }
    return loot_jobj;
}

json::value MapToValue(const Map& map) {
    json::value jv{
        {"id", *map.GetId()},
        {"name", map.GetName()},
        {"roads", json::value_from(map.GetRoads())},
        {"buildings", json::value_from(map.GetBuildings())},
        {"offices", json::value_from(map.GetOffices())},
        {"lootTypes", map.GetLootTypesInfo().AsJsonArray()},
    };
    if(auto speed = map.GetDogSpeed(); speed.has_value()) {
        jv.as_object().emplace("dogSpeed", *speed);
    }
    if(auto bag_cap = map.GetBagCapacity(); bag_cap.has_value()) {
        jv.as_object().emplace("bagCapacity", *bag_cap);
    }
    return jv;
}

std::string PrintPlayerList(const std::vector<app::PlayerPtr>& players) {
    std::stringstream ss;
    print_json(ss, std::move(MakePlayerListJson(players)));

    return ss.str();
}

std::string PrintGameState(const app::PlayerPtr player, const std::shared_ptr<app::GameInterface>& game_app) {
    std::stringstream ss;
    json::object game_state;
    game_state.emplace("players", std::move(MakePlayerStateJson(game_app->GetPlayerList(player)))
    );

    game_state.emplace("lostObjects", std::move(MakeLostObjectsJson(game_app->GetLootList(player)))
    );

    print_json(ss, game_state);
    return ss.str();
}

Map ParseMap(const json::value& map_json) {
    //if keys can be absent, use 'if (const auto ptr = map_obj.if_contains())'
    Map map = value_to<Map>(map_json);
    const auto& map_obj = map_json.as_object();

    //Add roads
    for(const auto& road_jv : map_json.at(JsonKeys::roads).as_array()) {
        map.AddRoad(value_to<Road>(road_jv));
    }

    //Add buildings
    for(const auto& bld_jv : map_json.at(JsonKeys::buildings).as_array()) {
        map.AddBuilding(value_to<Building>(bld_jv));
    }

    //Add offices
    for(const auto& offc_jv : map_json.at(JsonKeys::offices).as_array()) {
        map.AddOffice(value_to<Office>(offc_jv));
    }

    //Add LootItem info
    const auto loot_info_json_ptr = map_json.as_object().if_contains(JsonKeys::loot_types);
    if(loot_info_json_ptr) {
        map.AddLootInfo(std::move(loot_info_json_ptr->as_array()));
    } else {
        //empty array
        map.AddLootInfo(json::array{});
    }
    return map;
}

//Needs game ref for default values if not present in config
void ProcessOptionalMapParams(const json::value& map_json, Map& map, const Game& game) {
    const auto& map_obj = map_json.as_object();
    //Add speed, if specified in config
    if(auto it = map_obj.find(JsonKeys::dog_speed); it != map_obj.end()) {
        map.SetDogSpeed(it->value().as_double());
    }
    //Add capacity, if specified in config
    if(auto it = map_obj.find(JsonKeys::bag_cap); it != map_obj.end()) {
        map.SetBagCapacity(it->value().as_int64());
    }
}

void ProcessOptionalGameParams(const json::object game_obj, Game& game) {
    //Modify default speed, if specified in config
    if(auto it = game_obj.find(JsonKeys::dog_speed_dflt); it != game_obj.end()) {
        game.ModifyDefaultDogSpeed(it->value().as_double());
    }

    //Modify default bag capacity, if specified in config
    if(auto it = game_obj.find(JsonKeys::bag_cap_dflt); it != game_obj.end()) {
        game.ModifyDefaultBagCapacity(it->value().as_int64());
    }

    //LootItem gen config
    if(auto it = game_obj.find(JsonKeys::loot_gen); it != game_obj.end()) {
        //NB: Currently period is given as a double in seconds in json! Converting to chrono::duration in msec
        game.ConfigLootGen(util::ConvertSecToMsec(it->value().at("period").as_double())
                                 , it->value().at("probability").as_double()
        );
    }
}

const char ParseMove(const std::string& request_body) {
    auto j_obj = json::parse(request_body).as_object();
    if(auto it = j_obj.find("move"); it != j_obj.end()) {
        auto mv_cmd = it->value().as_string();
        return mv_cmd.empty() ? char{} : mv_cmd[0];
    }
    throw std::invalid_argument("cannot parse move command");
}

//Return time in seconds, assume timeDelta is always in ms
model::TimeMs ParseTick(const std::string& request_body) {
    auto j_obj = json::parse(request_body).as_object();
    if(!j_obj.at("timeDelta").as_int64()) {
        throw std::invalid_argument("expected a number for time");
    }
    return model::TimeMs{j_obj.at("timeDelta").as_int64()};
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    try {
        // Загрузить содержимое файла json_path, например, в виде строки
        std::ifstream input_file(json_path);
        std::string input(std::istreambuf_iterator<char>(input_file), {});

        // Распарсить строку как JSON, используя boost::json::parse
        auto game_config_json = json::parse(input);

        const auto map_array_ptr = game_config_json.as_object().if_contains(JsonKeys::maps);
        if(!map_array_ptr) {
            throw std::logic_error("No maps found in json file");
        }

        // Загрузить модель игры из файла
        Game game;
        ProcessOptionalGameParams(game_config_json.as_object(), game);

        for(const auto& json_map : map_array_ptr->as_array()) {
            auto new_map = ParseMap(json_map);

            ProcessOptionalMapParams(json_map, new_map, game);
            game.AddMap(std::move(new_map));
        }
        return game;
    }
    catch(std::exception& ex) {
        //TODO: Switch to boost error?
        //https://www.boost.org/doc/libs/1_85_0/libs/filesystem/doc/reference.html#Class-filesystem_error
        json::object additional_data{
            {"code", 1},
            {"text", ex.what()},
            {"where", "LoadGame"}
        };

        BOOST_LOG_TRIVIAL(info) << logging::add_value(log_message, "error")
                                << logging::add_value(log_msg_data, additional_data);
        return {};
    }

}
} // namespace json_loader


//===========================================================
//================= Tag Invoke overloads ====================
namespace geom {
//TODO: When throws errors from here, it doesn't get reported
void tag_invoke(const json::value_from_tag&, json::value& jv, Point2D const& pt) {
    jv = json::array{
        pt.x, pt.y
    };
}

Point2D tag_invoke(const json::value_to_tag<Point2D>&, json::value const& jv) {
    const auto& arr = jv.as_array();
    Point2D pt(value_to<double>(arr.at(0)), value_to<double>(arr.at(1)));
    return pt;
}

void tag_invoke(const json::value_from_tag&, json::value& jv, Vec2D const& speed) {
    jv = json::array{
        static_cast<double>(speed.x),
        static_cast<double>(speed.y)
    };
}

Vec2D tag_invoke(const json::value_to_tag<Vec2D>&, json::value const& jv) {
    auto ja = jv.as_array();
    return Vec2D(value_to<double>(jv.at(0)), value_to<double>(jv.at(1)));
}
} //namespace geom

namespace model {
void tag_invoke(const json::value_from_tag&, json::value& jv, Point const& pt) {
    jv = json::array{
        static_cast<int>(pt.x),
        static_cast<int>(pt.y)
    };
}

Point tag_invoke(const json::value_to_tag<Point>&, json::value const& jv) {
    const auto& arr = jv.as_array();
    return {value_to<int>(arr.at(0)), value_to<int>(arr.at(1))};
}

void tag_invoke(const json::value_from_tag&, json::value& jv, Direction const& dir) {
    jv = std::string{static_cast<char>(dir)};
}

Direction tag_invoke(const json::value_to_tag<Direction>&, json::value const& jv) {
    Direction dir(value_to<Direction>(jv.at("dir")));
    return dir;
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

//BagContent Items
//TODO: can split into LootItem tag invoke & BagItems. Can you use array(bag.begin, bag.end) constructor?
void tag_invoke(const json::value_from_tag&, json::value& jv, Dog::BagContent const& bag) {
    json::array ja;
    for(const auto& item : bag) {
        ja.emplace_back(json::object{
            {"id", item.type},
            {"type", item.id}
        });
    }
    jv = std::move(ja);
}
}  // namespace model


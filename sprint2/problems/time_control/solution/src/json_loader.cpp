#include "json_loader.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace json_loader {
namespace logging = boost::log;
using namespace model;
using namespace std::literals;

static constexpr json::string_view MAPS_KEY = "maps";
static constexpr json::string_view ROADS_KEY = "roads";
static constexpr json::string_view BUILDINGS_KEY = "buildings";
static constexpr json::string_view OFFICES_KEY = "offices";

//https://live.boost.org/doc/libs/1_83_0/libs/json/doc/html/json/examples.html
void pretty_print( std::ostream& os, json::value const& jv, std::string* indent = nullptr )
{
    os << json::serialize(jv);
    /*
    std::string indent_;
    if(! indent)
        indent = &indent_;
    switch(jv.kind())
    {
        case json::kind::object:
        {
            os << "{\n";
            indent->append(2, ' ');
            auto const& obj = jv.get_object();
            if(! obj.empty())
            {
                auto it = obj.begin();
                for(;;)
                {
                    os << *indent << json::serialize(it->key()) << ": ";
                    pretty_print(os, it->value(), indent);
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

        case json::kind::array:
        {
            os << "["; //\n";
            //indent->append(4, ' ');
            auto const& arr = jv.get_array();
            if(! arr.empty())
            {
                auto it = arr.begin();
                for(;;)
                {
                    std::string zero_indent;
                    //os << *indent;
                    pretty_print( os, *it, &zero_indent);
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

        case json::kind::string:
        {
            os << json::serialize(jv.get_string());
            break;
        }

        case json::kind::uint64:
            os << jv.get_uint64();
            break;

        case json::kind::int64:
            os << jv.get_int64();
            break;

        case json::kind::double_:
            os << jv.get_double();
            break;

        case json::kind::bool_:
            if(jv.get_bool())
                os << "true";
            else
                os << "false";
            break;

        case json::kind::null:
            os << "null";
            break;
    }

//    if(indent->empty())
//        os << "\n";
     */
}

std::string PrintMapList(const model::Game::Maps& map_list) {
    json::array map_list_js;

    for(const auto& map : map_list) {
        map_list_js.push_back(json::value_from(map));
    }
    std::stringstream ss;
    pretty_print(ss, map_list_js);

    return ss.str();
    //return json::serialize(map_list_js);
}

std::string PrintMap(const Map& map) {
    std::stringstream ss;
    pretty_print(ss, MapToValue(map));

    return ss.str();
}

std::string PrintPlayerList(const std::vector<app::PlayerPtr>& players) {
    json::object result;
    for(const auto& p_ptr : players) {
        result.emplace(std::to_string(p_ptr->GetId()),
                       json::object{
                           {"name", std::string(p_ptr->GetDog()->GetName())}
                       }
        );
    }
    std::stringstream ss;
    pretty_print(ss, result);

    return ss.str();
}

std::string PrintPlayerState(const std::vector<app::PlayerPtr>& players) {
    json::object player_state;
    for(const auto& p_ptr : players) {
        auto dog = p_ptr->GetDog();
        const auto pos_ja = json::value_from(dog->GetPos()).as_array();
        const auto speed_ja = json::value_from(dog->GetSpeed()).as_array();
        const auto dir_jv = json::value_from(dog->GetDir());

        player_state.emplace(std::to_string(p_ptr->GetId()),
                             json::object{
                                 {"pos", pos_ja},
                                 {"speed", speed_ja},
                                 {"dir", dir_jv}
                             }
        );
    }
    std::stringstream ss;
    pretty_print(ss, json::object{{"players", player_state}});
    std::cerr << ss.str() << std::endl;
    return ss.str();
}

//json::object MakePlayerListJson(const std::vector<app::PlayerPtr>& plist) {
//json::object MakePlayerStateJson(const std::vector<app::PlayerPtr>& plist)

Map ParseMap(const json::value& map_json) {
    //if keys can be absent, use 'if (const auto ptr = map_obj.if_contains())'
    Map map = value_to<Map>(map_json);

    //Add speed, if specified in config
    const auto& map_obj = map_json.as_object();
    if(auto it = map_obj.find("dogSpeed"); it != map_obj.end()) {
        map.SetDogSpeed(it->value().as_double());
    }

    //Add roads
    for(const auto& road_jv : map_json.at(ROADS_KEY).as_array()) {
        map.AddRoad(value_to<Road>(road_jv));
    }

    //Add buildings
    for(const auto& bld_jv : map_json.at(BUILDINGS_KEY).as_array()) {
        map.AddBuilding(value_to<Building>(bld_jv));
    }

    //Add offices
    for(const auto& offc_jv : map_json.at(OFFICES_KEY).as_array()) {
        map.AddOffice(value_to<Office>(offc_jv));
    }
    return map;
}

json::value MapToValue(const Map& map) {
    return {
        //{"dogSpeed", map.GetDogSpeed()},
        {"id", *map.GetId()},
        {"name", map.GetName()},
        {"roads", json::value_from(map.GetRoads())},
        {"buildings", json::value_from(map.GetBuildings())},
        {"offices", json::value_from(map.GetOffices())},
    };
}

const char ParseMove(const std::string& request_body) {
    auto j_obj = json::parse(request_body).as_object();
    if(auto it = j_obj.find("move"); it != j_obj.end()) {
        auto mv_cmd = it->value().as_string();

        return mv_cmd.empty() ? char{} : mv_cmd[0];
    }
    return app::MOVE_CMD_ERR;
}

//Return time in seconds, assume timeDelta is always in ms
model::Time ParseTick(const std::string& request_body) {
    auto j_obj = json::parse(request_body).as_object();
    if(!j_obj.at("timeDelta").as_int64()) {
        //TODO: better way?
        throw std::invalid_argument("expected a number for time");
    }
    return 1.0 * j_obj.at("timeDelta").as_int64() / 1000;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    try {
        // Загрузить содержимое файла json_path, например, в виде строки
        std::ifstream input_file(json_path);

        //TODO: Add file opennign error handling
        std::string input(std::istreambuf_iterator<char>(input_file), {});

        // Распарсить строку как JSON, используя boost::json::parse
        auto doc = json::parse(input);
        const auto map_array_ptr = doc.as_object().if_contains(MAPS_KEY);

        if(!map_array_ptr) {
            throw std::logic_error("No maps in file");
        }

        // Загрузить модель игры из файла
        Game game;

        //Set default speed, if specified in config
        const auto& j_obj = doc.as_object();
        if(auto it = j_obj.find("defaultDogSpeed"); it != j_obj.end()) {
            game.SetDefaultDogSpeed(it->value().as_double());
        }

        for(const auto& json_map : map_array_ptr->as_array()) {
            game.AddMap(std::move(ParseMap(json_map)));
        }

        return game;
    } catch(std::exception& ex) {
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

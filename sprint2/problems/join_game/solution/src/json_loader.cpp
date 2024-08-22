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

    std::string PrintErrorMsgJson(json::string_view code, json::string_view message) {
        json::value jv{
            {"code", code},
            {"message", message}
        };
        return json::serialize(jv);
    }

    std::string PrintMapList(const Game &game) {
        json::array map_list_js;

        for (const auto &map : game.GetMaps()) {
            map_list_js.push_back(json::value_from(map));
        }
        return json::serialize(map_list_js);
    }

    std::string PrintMap(const Map &map) {
        return json::serialize(MapToValue(map));
    }

    Map ParseMap(const json::value &map_json) {
        //if keys can be absent, use 'if (const auto ptr = map_obj.if_contains())'
        Map map = value_to<Map>(map_json);

        //Add roads
        for (const auto &road_jv : map_json.at(ROADS_KEY).as_array()) {
            map.AddRoad(value_to<Road>(road_jv));
        }

        //Add buildings
        for (const auto &bld_jv : map_json.at(BUILDINGS_KEY).as_array()) {
            map.AddBuilding(value_to<Building>(bld_jv));
        }

        //Add offices
        for (const auto &offc_jv : map_json.at(OFFICES_KEY).as_array()) {
            map.AddOffice(value_to<Office>(offc_jv));
        }
        return map;
    }


    model::Game LoadGame(const std::filesystem::path &json_path) {
        try {
        // Загрузить содержимое файла json_path, например, в виде строки
        std::ifstream input_file(json_path);

        //TODO: Add file opennign error handling
        std::string input(std::istreambuf_iterator<char>(input_file), {});

        // Распарсить строку как JSON, используя boost::json::parse
        auto doc = json::parse(input);

        const auto map_array_ptr = doc.as_object().if_contains(MAPS_KEY);

        if (!map_array_ptr) {
            throw std::logic_error("No maps in file");
        }

        // Загрузить модель игры из файла
        Game game;


        for (const auto &json_map: map_array_ptr->as_array()) {
            game.AddMap(ParseMap(json_map));
        }

            return game;
        } catch (std::exception &ex) {
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

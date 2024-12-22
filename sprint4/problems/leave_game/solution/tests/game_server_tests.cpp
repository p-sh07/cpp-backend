#include <catch2/catch_test_macros.hpp>
#include <random>
#include <iostream>

#include "../src/application.h"
#include "../src/model.h"
#include "../src/loot_generator.h"
#include "../src/postgres.h"
#include "../src/json_loader.h"

using namespace std::literals;

char SelectRandomMove() {
    std::string move_chars = "UDLR"s;
    return move_chars[std::rand() % move_chars.size()];
}

SCENARIO("Game model testing") {
    GIVEN("a game_app") {
        namespace net = boost::asio;
        //auto pstat_db = std::make_shared<postgres::PlayerStatsImpl>("postgres://postgres:Mys3Cr3t@172.17.0.2:5432/playerdb"s, 4);

        auto game = std::make_shared<model::Game>(json_loader::LoadGame("../../data/config.json"));

        net::io_context ioc(static_cast<int>(4));
        // 2.1. При наличии сохраненного состояния, восстанавливаем данные из файла //TODO: Restore throw if unsuccessful
        auto game_app = std::make_shared<app::GameInterface>(ioc, game, nullptr, nullptr);

        WHEN("Adding 50 players and moving them 100 times for less than 15 sec") {
            std::unordered_map<const app::Token*, size_t> token_map;

            for(int i = 0; i < 100; ++i) {
                auto result = game_app->JoinGame("map1", "tribe" + std::to_string(i));
                token_map.emplace(result.token, result.player_id);
            }

            for(int i = 0; i < 100; ++i) {
                for(const auto& [token, _ ] : token_map) {
                    auto m = SelectRandomMove();
                    std::cerr << "Moving: " << m << "\n";
                    game_app->SetPlayerMovement(game_app->FindPlayerByToken(*token), SelectRandomMove());
                }
                //on map 1 retire time is 15 sec, use 7 sec
                game_app->AdvanceGameTime(model::TimeMs{15000u});
            }
        }
    }
}

using namespace std::literals;

SCENARIO("LootItem generation") {
    using loot_gen::LootGenerator;
    using TimeInterval = LootGenerator::TimeInterval;

    GIVEN("a loot generator") {
        LootGenerator gen{1s, 1.0};

        constexpr TimeInterval TIME_INTERVAL = 1s;

        WHEN("loot count is enough for every looter") {
            THEN("no loot is generated") {
                for (unsigned looters = 0; looters < 10; ++looters) {
                    for (unsigned loot = looters; loot < looters + 10; ++loot) {
                        INFO("loot count: " << loot << ", looters: " << looters);
                        REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == 0);
                    }
                }
            }
        }

        WHEN("number of looters exceeds loot count") {
            THEN("number of loot is proportional to loot difference") {
                for (unsigned loot = 0; loot < 10; ++loot) {
                    for (unsigned looters = loot; looters < loot + 10; ++looters) {
                        INFO("loot count: " << loot << ", looters: " << looters);
                        REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == looters - loot);
                    }
                }
            }
        }
    }

    GIVEN("a loot generator with some probability") {
        constexpr TimeInterval BASE_INTERVAL = 1s;
        LootGenerator gen{BASE_INTERVAL, 0.5};

        WHEN("time is greater than base interval") {
            THEN("number of generated loot is increased") {
                CHECK(gen.Generate(BASE_INTERVAL * 2, 0, 4) == 3);
            }
        }

        WHEN("time is less than base interval") {
            THEN("number of generated loot is decreased") {
                const auto time_interval
                    = std::chrono::duration_cast<TimeInterval>(std::chrono::duration<double>{
                        1.0 / (std::log(1 - 0.5) / std::log(1.0 - 0.25))});
                CHECK(gen.Generate(time_interval, 0, 4) == 1);
            }
        }
    }

    GIVEN("a loot generator with custom random generator") {
        LootGenerator gen{1s, 0.5, [] {
            return 0.5;
        }};
        WHEN("loot is generated") {
            THEN("number of loot is proportional to random generated values") {
                const auto time_interval
                    = std::chrono::duration_cast<TimeInterval>(std::chrono::duration<double>{
                        1.0 / (std::log(1 - 0.5) / std::log(1.0 - 0.25))});
                CHECK(gen.Generate(time_interval, 0, 4) == 0);
                CHECK(gen.Generate(time_interval, 0, 4) == 1);
            }
        }
    }
}



//Gathering Test
TEST_CASE("Basic Gather test", "[LootGathering]") {
    //setup sessiion
    //gen object
    //collect object
    //return object
    SECTION("collecting object") {
        //...
    }

    SECTION("returning object") {
        //...
    }

    //etc...
}

///Main Example
/**
 void DownloadTestResources();

int main(int argc, char* argv[]) {
    std::cout << "Downloading files for test..." << std::endl;
    DownloadTestResources();

    int result = Catch::Session().run(argc, argv);

    return result;
}

 ///Часть параметров командной строки можно обработать вручную. Но так делать не рекомендуется. Лучше расширить парсер библиотеки Catch2. Не будем останавливаться на этом моменте, он описан в документации.
 ///https://github.com/catchorg/Catch2/blob/devel/docs/own-main.md#adding-your-own-command-line-options
*/

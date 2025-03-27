#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "../src/model.h"
#include "../src/json_loader.h"
#include "../src/state_serialization.h"

using namespace model;
using namespace std::literals;
/*
static constexpr auto GAME_CONFIG = "../../data/config.json"sv;

namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

SCENARIO_METHOD(Fixture, "Point serialization") {
    GIVEN("A point") {
        const geom::Point2D p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                geom::Point2D restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog Serialization") {
    GIVEN("a dog") {
        const auto dog = [] {
            Dog dog{Dog::Id{42u}, {42.2, 12.5}, 0.6, Dog::Name{"Pluto"s}, 3};
            dog.AddScore(42);
            CHECK(dog.TryCollectItem({LootItem::Id{10u}, 2u, 1u, true}));
            dog.SetDirection(Direction::EAST);
            dog.SetSpeed({2.3, -1.2});
            return dog;
        }();

        WHEN("dog is serialized") {
            {
                serialization::DogRepr repr{dog};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(dog.GetId() == restored.GetId());
                CHECK(dog.GetTag() == restored.GetTag());
                CHECK(dog.GetPos() == restored.GetPos());
                CHECK(dog.GetSpeed() == restored.GetSpeed());
                CHECK(dog.GetBagCap() == restored.GetBagCap());
                // CHECK(dog.GetBag() == restored.GetBag());
            }
        }
    }
}


SCENARIO_METHOD(Fixture, "Session serialization") {
    auto game = std::make_shared<model::Game>(json_loader::LoadGame(GAME_CONFIG));
    GIVEN("a session") {

        const auto session = [&game] {
            app::Session sessn(0u, game->FindMap(model::Map::Id{"map1"s}), game->GetSettings());
            Dog dog{Dog::Id{42u}, {42.2, 12.5}, 0.6, Dog::Tag{"Pluto"s}, 3};
            Dog dog2{Dog::Id{0u}, {49.1, 12.12}, 0.6, Dog::Tag{"Mercury"s}, 3};
            dog.AddScore(42);
            dog2.AddScore(18);

            sessn.AddDog(std::move(dog));
            sessn.AddDog(std::move(dog2));

            sessn.AddLootItem(0u, 0u, {1.0, 1.0});
            sessn.AddLootItem(1u, 1u, {2.0, 2.0});

            return sessn;
        }();

        WHEN("session is serialized") {
            {
                serialization::SessionRepr repr{session};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::SessionRepr repr;
                input_archive >> repr;
                const auto restored_sessn = repr.Restore(game);
                auto restored_dog = restored_sessn.GetDog(42u);

                CHECK(42u == restored_dog->GetId());
                CHECK(Dog::Tag{"Pluto"s} == restored_dog->GetTag());

                //TODO: etc.

            }
        }
    }
}


SCENARIO_METHOD(Fixture, "Psm serialization") {
    auto game = std::make_shared<model::Game>(json_loader::LoadGame(GAME_CONFIG));
    GIVEN("a psm") {

        const auto psm = [&game] {
            app::PlayerSessionManager psm(game);

            psm.CreatePlayer(Map::Id{"map1"s}, Dog::Tag{"pluto"s});
            psm.CreatePlayer(Map::Id{"map1"s}, Dog::Tag{"mercury"s});

            return psm;
        }();

        WHEN("psm is serialized") {
            {
                serialization::PsmRepr repr{psm};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::PsmRepr repr;
                input_archive >> repr;
                const auto restored_psm = repr.Restore(game);
                auto restored_dog_count = (restored_psm.GetAllPlayers()).size();

                CHECK(restored_dog_count == 2u);
            }
        }
    }
}
*/
#define _USE_MATH_DEFINES

#include <catch2/catch_test_macros.hpp>
//#include <catch2/matchers/catch_matchers.hpp>
//#include <catch2/matchers/catch_matchers_all.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "../src/collision_detector.h"
#include <iostream>
#include <sstream>
#include <vector>

using namespace std::literals;

static constexpr double DBL_EQUALS_MARGIN = 10e-10;

namespace collision_detector {
bool operator==(const GatheringEvent& lhs, const GatheringEvent& rhs) {
    return lhs.item_id == rhs.item_id
        && lhs.gatherer_id == rhs.gatherer_id
        && std::abs(lhs.sq_distance - rhs.sq_distance) < DBL_EQUALS_MARGIN
        && std::abs(lhs.time - rhs.time) < DBL_EQUALS_MARGIN;
}
}// collision_detector

using namespace collision_detector;

namespace Catch {
template<>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.item_id << "," << value.gatherer_id << "," << value.sq_distance << "," << value.time << ")";

        return tmp.str();
    }
};
}  // namespace Catch


class TestGathererProvider : public ItemGathererProvider {
 public:
    TestGathererProvider(const std::vector<Item>& items, const std::vector<Gatherer>& gatherers)
    : items_(items)
    , gatherers_(gatherers)
    {}

    size_t ItemsCount() const override {
        return items_.size();
    }

    Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }
 private:
    const std::vector<Item> items_;
    const std::vector<Gatherer> gatherers_;
};

//Vector<GatheringEvent> custom matcher
using EventVector = std::vector<GatheringEvent>;

struct EventVectorMatcher : Catch::Matchers::MatcherGenericBase {

    EventVectorMatcher(const EventVector& vec)
    : vec_(vec)
    {}

    bool match(const EventVector& other) const {
        if(vec_.size() != other.size()) {
            return false;
        }

        for(size_t i = 0; i < vec_.size(); ++i) {
            if(vec_[i] != other[i]) {
                return false;
            }
        }
        return true;
    }

    std::string describe() const override {
        return "\nMust Equal:\n" + Catch::rangeToString(vec_);
    }

 private:
    const EventVector vec_;
};

auto EventsMatcher(const EventVector& vec) -> EventVectorMatcher {
    return EventVectorMatcher{vec};
}

//std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider)
/**
Своими тестами проверьте, что:
- функция FindGatherEvents детектирует все события столкновений,
- функция FindGatherEvents не детектирует лишних событий,
- события идут в хронологическом порядке,
- события имеют правильные данные — время, индексы, расстояние.
*/

/* Info:
 * struct GatheringEvent {
    size_t item_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
};
 */

TEST_CASE("One gatherer, moving in a straight line", "[CollisionDetect]") {
    //Calculated manually:
    EventVector CorrectResult = {
        {0, 0, 0, 0.0},
        {1, 0, 0, 0.25},
        {2, 0, 0, 0.50},
        {3, 0, 0, 0.75},
        {4, 0, 0, 1.0},
    };

    //To use with function:
    const std::vector<Item> items {
        {{0,0}, 0.1},
        {{1,0}, 0.1},
        {{2,0}, 0.1},
        {{3,0}, 0.1},
        {{4,0}, 0.1},
    };

    const std::vector<Gatherer> gatherers {
        {{0,0}, {4,0}, 0.1},
    };

    TestGathererProvider tgp(items, gatherers);
    EventVector FunctionResult = FindGatherEvents(tgp);

    REQUIRE_THAT(FunctionResult, EventsMatcher(CorrectResult));
}

TEST_CASE("Two gatherers, moving in a straight line, not in sync", "[CollisionDetect]") {
    //Calculated manually:
    EventVector CorrectResult = {
        {0, 0, 0, 0.0},
        {1, 1, 0, 0.125},
        {1, 0, 0, 0.25},
        {2, 1, 0, 0.375},
        {2, 0, 0, 0.50},
        {3, 1, 0, 0.625},
        {3, 0, 0, 0.75},
        {4, 1, 0, 0.875},
        {4, 0, 0, 1.0},
    };

    //To use with function:
    const std::vector<Item> items {
        {{0,0}, 0.1},
        {{1,0}, 0.1},
        {{2,0}, 0.1},
        {{3,0}, 0.1},
        {{4,0}, 0.1},
    };

    const std::vector<Gatherer> gatherers {
        {{0,0}, {4,0}, 0.1},
        {{0.5,0}, {4.5,0}, 0.1},
    };

    TestGathererProvider tgp(items, gatherers);
    EventVector FunctionResult = FindGatherEvents(tgp);

    REQUIRE_THAT(FunctionResult, EventsMatcher(CorrectResult));
}

TEST_CASE("One gatherer, moving diagonally", "[CollisionDetect]") {
    //Calculated manually:
    EventVector CorrectResult = {
        {0, 0, 0, 0.0},
        {1, 0, 0, 0.25},
        {2, 0, 0, 0.50},
        {3, 0, 0, 0.75},
        {4, 0, 0, 1.0}
    };

    //To use with function:
    const std::vector<Item> items {
        {{0,0}, 0.1},
        {{1,1}, 0.1},
        {{2,2}, 0.1},
        {{3,3}, 0.1},
        {{4,4}, 0.1}
    };

    const std::vector<Gatherer> gatherers {
        {{0,0}, {4,4}, 0.1}
    };

    TestGathererProvider tgp(items, gatherers);
    EventVector FunctionResult = FindGatherEvents(tgp);

    REQUIRE_THAT(FunctionResult, EventsMatcher(CorrectResult));
}

TEST_CASE("Two gatherers, different objects", "[CollisionDetect]") {
    //Calculated manually:
    EventVector CorrectResult = {
        {5, 0, 0, 0.0},   // straight gatherer
        {1, 1, 0, 0.125}, // diag. gatherer
        {6, 0, 0, 0.25},
        {2, 1, 0, 0.375},
        {7, 0, 0, 0.50},
        {3, 1, 0, 0.625},
        {8, 0, 0, 0.75},
        {4, 1, 0, 0.875},
        {9, 0, 0, 1.0},
    };

    //To use with function:
    const std::vector<Item> items {
        {{-1,-1}, 0.1}, // id = 0
        {{1,1}, 0.1},
        {{2,2}, 0.1},
        {{3,3}, 0.1},
        {{4,4}, 0.1}, // id = 4

        {{0,0}, 0.1}, // id = 5
        {{1,0}, 0.1},
        {{2,0}, 0.1},
        {{3,0}, 0.1},
        {{4,0}, 0.1}, // id = 9
    };

    const std::vector<Gatherer> gatherers {
        {{0,0}, {4,0}, 0.1},
        {{0.5,0.5}, {4.5,4.5}, 0.1}
    };

    TestGathererProvider tgp(items, gatherers);
    EventVector FunctionResult = FindGatherEvents(tgp);

    REQUIRE_THAT(FunctionResult, EventsMatcher(CorrectResult));
}

TEST_CASE("One gatherer, negative coord", "[CollisionDetect]") {
    //Calculated manually:
    EventVector CorrectResult = {
        {0, 0, 0, 0.0},
        {1, 0, 0, 0.25},
        {2, 0, 0, 0.50},
        {3, 0, 0, 0.75},
        {4, 0, 0, 1.0}
    };

    //To use with function:
    const std::vector<Item> items {
        {{0,0}, 0.1},
        {{-1,-1}, 0.1},
        {{-2,-2}, 0.1},
        {{-3,-3}, 0.1},
        {{-4,-4}, 0.1}
    };

    const std::vector<Gatherer> gatherers {
        {{0,0}, {-4,-4}, 0.1}
    };

    TestGathererProvider tgp(items, gatherers);
    EventVector FunctionResult = FindGatherEvents(tgp);

    REQUIRE_THAT(FunctionResult, EventsMatcher(CorrectResult));
}

TEST_CASE("One gatherer, no collect", "[CollisionDetect]") {
    //To use with function:
    const std::vector<Item> items {
        {{-1,-1}, 0.1},
        {{5,1}, 0.2},
        {{-3,-1}, 0.1},
        {{1,-4}, 0.1}
    };

    const std::vector<Gatherer> gatherers {
        {{0,0}, {4,4}, 0.1}
    };

    TestGathererProvider tgp(items, gatherers);
    EventVector FunctionResult = FindGatherEvents(tgp);

    REQUIRE(FunctionResult.empty());
}

TEST_CASE("One gatherer, no move", "[CollisionDetect]") {
    //To use with function:
    const std::vector<Item> items {
        {{0,0}, 0.1},
        {{1,1}, 0.1},
        {{2,2}, 0.1},
        {{3,3}, 0.1},
        {{4,4}, 0.1}
    };

    const std::vector<Gatherer> gatherers {
        {{1,1}, {1,1}, 0.1}
    };

    TestGathererProvider tgp(items, gatherers);
    EventVector FunctionResult = FindGatherEvents(tgp);

    REQUIRE(FunctionResult.empty());
}
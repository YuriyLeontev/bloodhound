#define _USE_MATH_DEFINES

#include <cmath>
#include <functional>
#include <sstream>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "../src/model/collision_detector.h"

namespace Catch {
template<>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << " " <<  value.item_id <<  " " << value.sq_distance << " " << value.time << ")";

        return tmp.str();
    }
};
}  // namespace Catch

namespace {

template <typename Range, typename Predicate>
struct EqualsRangeMatcher : Catch::Matchers::MatcherGenericBase {
    EqualsRangeMatcher(Range const& range, Predicate predicate)
        : range_{range}
        , predicate_{predicate} {
    }

    template <typename OtherRange>
    bool match(const OtherRange& other) const {
        using std::begin;
        using std::end;

        return std::equal(begin(range_), end(range_), begin(other), end(other), predicate_);
    }

    std::string describe() const override {
        return "Equals: " + Catch::rangeToString(range_);
    }

private:
    const Range& range_;
    Predicate predicate_;
};

template <typename Range, typename Predicate>
auto EqualsRange(const Range& range, Predicate prediate) {
    return EqualsRangeMatcher<Range, Predicate>{range, prediate};
}

class CompareEvents {
public:
    bool operator()(const collision_detector::GatheringEvent& l,
                    const collision_detector::GatheringEvent& r) {
        if (l.gatherer_id != r.gatherer_id || l.item_id != r.item_id) 
            return false;

        static const double eps = 1e-10;

        if (std::abs(l.sq_distance - r.sq_distance) > eps) {
            return false;
        }

        if (std::abs(l.time - r.time) > eps) {
            return false;
        }
        return true;
    }
};

double calcTime(geom::Vec2D a, geom::Vec2D b, geom::Vec2D c){
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    
    return u_dot_v / v_len2;
}

}

SCENARIO("Collision detection") {
    WHEN("no items") {
        collision_detector::VectorItemGathererProvider provider{
            {}, {{{1, 2}, {4, 2}, 5.}, {{0, 0}, {10, 10}, 5.}, {{-5, 0}, {10, 5}, 5.}}};
        THEN("No events") {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK(events.empty());
        }
    }
    WHEN("no gatherers") {
        collision_detector::VectorItemGathererProvider provider{
            {{{1, 2}, 5.}, {{0, 0}, 5.}, {{-5, 0}, 5.}}, {}};
        THEN("No events") {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK(events.empty());
        }
    
    }
    WHEN("multiple items on a way of gatherer") {
        collision_detector::VectorItemGathererProvider provider{{
            {{1.1,0.61},.1}, // YES
            {{2, 1.27}, .1}, // YES
            {{5, 1},    .1}, // YES
            {{8,  0.8}, .1}, // YES
            {{-1, 0.15},.1}, // NO
            {{5, 5},    .1}, // NO
            {{0, 0},    .1}, // NO
            {{6, 1.6},  .1}, // NO
            }, {
            {{1, 1}, {8, 1}, 0.4},
        }};
        THEN("Gathered items in right order") {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK_THAT(
                events,
                EqualsRange(std::vector{
                    collision_detector::GatheringEvent{0, 0,0.39*0.39, calcTime({1,1},{8,1},{1.1,0.61})},
                    collision_detector::GatheringEvent{1, 0,0.27*0.27, calcTime({1,1},{8,1},{2,1.27})},
                    collision_detector::GatheringEvent{2, 0,0.*0.,     calcTime({1,1},{8,1},{5,1})},
                    collision_detector::GatheringEvent{3, 0,0.2*0.2,   calcTime({1,1},{8,1},{8,0.8})},
                }, CompareEvents()));
        }
    }
    WHEN("multiple gatherers and one item") {
        collision_detector::VectorItemGathererProvider provider{{
                                                {{5, 5}, .1},
                                            },
                                            {
                                                {{-1, 0}, {6, 0}, 1.1},  // NO
                                                {{0, 6}, {0, -1}, 1.1},  // NO
                                                {{0, 0}, {6, 10}, 0.5},  // NO
                                                {{0, 10}, {10, 0}, 0.4}, // YES
                                            }
        };
        THEN("Item gathered by faster gatherer") {
            auto events = collision_detector::FindGatherEvents(provider);
            CHECK(events.front().gatherer_id == 3);
        }
    }
    WHEN("Gatherers stay put") {
        collision_detector::VectorItemGathererProvider provider{{
                                                {{0, 0}, 1.},
                                            },
                                            {
                                                {{0, 0}, {0, 0}, 1.},
                                                {{-5, 0}, {-5, 0}, 1.},
                                                {{0, 0}, {0, 0}, 1.},
                                                {{-10, 10}, {-10, 10}, 100}
                                            }
        };
        THEN("No events detected") {
            auto events = collision_detector::FindGatherEvents(provider);

            CHECK(events.empty());
        }
    }       
}
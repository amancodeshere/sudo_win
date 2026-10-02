#include "sudo_win/geometry/geometry.h"

#include <catch2/catch.hpp>

TEST_CASE("toroidal geometry") {
    SECTION("coordinates wrap at both boundaries") {
        CHECK(sudo_win::geometry::wrap(-1, 10) == 9);
        CHECK(sudo_win::geometry::wrap(10, 10) == 0);
    }

    SECTION("axis distance crosses the closest boundary") {
        CHECK(sudo_win::geometry::toroidal_axis_distance(0, 9, 10) == 1);
    }

    SECTION("two-dimensional distances use toroidal topology") {
        CHECK(sudo_win::geometry::toroidal_manhattan({0, 0}, {9, 9}, 10, 10) == 2);
        CHECK(sudo_win::geometry::toroidal_chebyshev({0, 0}, {9, 8}, 10, 10) == 2);
    }

    SECTION("direction indices follow engine edge order") {
        CHECK(sudo_win::geometry::direction_index(unswbc::Direction::WEST) == 3);
    }
}

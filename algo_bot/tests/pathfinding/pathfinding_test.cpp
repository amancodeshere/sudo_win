#include "sudo_win/pathfinding/pathfinding.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("visible pathfinding") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto pathfinding = sudo_win::Pathfinding{};

    SECTION("flood fill reaches the entire empty vision window") {
        CHECK(pathfinding.visible_reachable_area(fixture.controller, {6, 5}) == 49);
    }

    SECTION("BFS finds the nearest visible pearl") {
        fixture.controller.get_tile({7, 5})->pearl = true;
        CHECK(pathfinding.visible_pearl_distance(fixture.controller, {6, 5}) == 1);
    }

    SECTION("BFS routes around a blocked direct edge") {
        fixture.controller.get_tile({7, 5})->pearl = true;
        fixture.controller.get_tile({6, 5})->get_edge(unswbc::Direction::EAST)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
        fixture.controller.get_tile({7, 5})->get_edge(unswbc::Direction::WEST)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
        CHECK(pathfinding.visible_pearl_distance(fixture.controller, {6, 5}) == 3);
    }
}

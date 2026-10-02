#include "sudo_win/pathfinding/pathfinding.h"
#include "sudo_win/world/world_model.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("visible pathfinding") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto pathfinding = sudo_win::Pathfinding{};

    SECTION("flood fill reaches the entire empty vision window") {
        CHECK(pathfinding.visible_reachable_area(fixture.controller, {6, 5}) == 49);
    }

    SECTION("BFS finds the nearest visible pearl") {
        fixture.tile({7, 5}).pearl = true;
        CHECK(pathfinding.visible_pearl_distance(fixture.controller, {6, 5}) == 1);
    }

    SECTION("BFS routes around a blocked direct edge") {
        fixture.tile({7, 5}).pearl = true;
        fixture.tile({6, 5}).get_edge(unswbc::Direction::EAST)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
        fixture.tile({7, 5}).get_edge(unswbc::Direction::WEST)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
        CHECK(pathfinding.visible_pearl_distance(fixture.controller, {6, 5}) == 3);
    }
}

TEST_CASE("remembered routes retain targets and reject stale or blocked pearls") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto& tile : fixture.controller.vision.tiles) {
        tile.pearl_time = -1;
    }
    fixture.tile({8, 5}).pearl = true;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto pathfinding = sudo_win::Pathfinding{};
    SECTION("known pearls outside current vision remain routable") {
        auto tiles = fixture.controller.vision.tiles;
        tiles.erase(std::remove_if(tiles.begin(), tiles.end(), [](auto const& tile) {
            return tile.get_position() == unswbc::Position{8, 5};
        }), tiles.end());
        fixture.controller.vision = unswbc::Vision{std::move(tiles)};
        auto const route = pathfinding.remembered_target(fixture.controller, world, 2);
        REQUIRE(route);
        CHECK(route->target == unswbc::Position{8, 5});
        CHECK(route->first_direction == unswbc::Direction::EAST);
        CHECK(route->distance == 3);
    }
    SECTION("old pearl sightings expire") {
        auto const route = pathfinding.remembered_target(fixture.controller, world, 100);
        REQUIRE(route);
        CHECK_FALSE(route->pearl);
    }
    SECTION("newly observed obstacles invalidate a direct route") {
        fixture.tile({5, 5}).get_edge(unswbc::Direction::EAST)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
        world.update(fixture.controller, fixture.game);
        auto const route = pathfinding.remembered_target(fixture.controller, world, 1);
        REQUIRE(route);
        CHECK(route->first_direction != unswbc::Direction::EAST);
    }
    SECTION("a preferred equally valuable target prevents direction switching") {
        fixture.tile({8, 5}).pearl = false;
        fixture.tile({7, 5}).pearl = true;
        fixture.tile({5, 7}).pearl = true;
        world.update(fixture.controller, fixture.game);
        auto const route = pathfinding.remembered_target(fixture.controller, world, 1, unswbc::Position{5, 7});
        REQUIRE(route);
        CHECK(route->target == unswbc::Position{5, 7});
        CHECK(route->first_direction == unswbc::Direction::SOUTH);
        fixture.tile({5, 7}).pearl = false;
        world.update(fixture.controller, fixture.game);
        auto const replaced = pathfinding.remembered_target(fixture.controller, world, 1, route->target);
        REQUIRE(replaced);
        CHECK(replaced->target == unswbc::Position{7, 5});
    }
}

TEST_CASE("resource routes yield to closer visible allies only on legal paths") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    for (auto& tile : fixture.controller.vision.tiles) {
        tile.pearl_time = -1;
    }
    fixture.tile({7, 5}).pearl = true;
    fixture.tile({5, 7}).pearl = true;
    fixture.tile({7, 4}).dragon_part = unswbc::DragonPart{
        {7, 4}, 2, unswbc::Team::A, unswbc::Direction::SOUTH, true};
    auto world = sudo_win::WorldModel{fixture.game};
    auto const route = [&] {
        world.update(fixture.controller, fixture.game);
        return sudo_win::Pathfinding{}.remembered_target(fixture.controller, world, 0,
                                                          unswbc::Position{7, 5});
    };
    SECTION("an ally's nearer pearl sends us to a separate resource") {
        auto const chosen = route();
        REQUIRE(chosen);
        CHECK(chosen->target == unswbc::Position{5, 7});
    }
    SECTION("a longer snake keeps growth priority over a small helper") {
        fixture.controller.length = 10;
        auto const chosen = route();
        REQUIRE(chosen);
        CHECK(chosen->target == unswbc::Position{7, 5});
    }
    SECTION("a queen keeps food priority regardless of helper length") {
        fixture.controller.head.dragon_id = 1;
        auto const chosen = route();
        REQUIRE(chosen);
        CHECK(chosen->target == unswbc::Position{7,5});
    }
    SECTION("a long helper yields to the fixed queen even when she is shorter") {
        fixture.controller.length = 10;
        fixture.tile({7,4}).dragon_part->dragon_id = 0;
        auto const chosen = route();
        REQUIRE(chosen);
        CHECK(chosen->target == unswbc::Position{5,7});
    }
    SECTION("walls prevent an apparent geometric claim") {
        fixture.tile({7, 4}).get_edge(unswbc::Direction::SOUTH)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
        auto const chosen = route();
        REQUIRE(chosen);
        CHECK(chosen->target == unswbc::Position{7, 5});
    }
    SECTION("enemy heads do not receive friendly resource priority") {
        fixture.tile({7, 4}).dragon_part->team = unswbc::Team::B;
        auto const chosen = route();
        REQUIRE(chosen);
        CHECK(chosen->target == unswbc::Position{7, 5});
    }
    SECTION("old ally sightings do not keep a resource reserved") {
        world.update(fixture.controller, fixture.game);
        fixture.tile({7, 4}).dragon_part.reset();
        auto const chosen = route();
        REQUIRE(chosen);
        CHECK(chosen->target == unswbc::Position{7, 5});
    }
}

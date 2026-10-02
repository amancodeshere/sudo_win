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

TEST_CASE("fresh queen sonar claims coordinate helper food and expire") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 6;
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    fixture.tile({7,5}).pearl = true;
    fixture.tile({5,7}).pearl = true;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    world.receive_report({sudo_win::MessageType::feeder,1,0,7,5,1},1);
    auto const route = sudo_win::Pathfinding{}.remembered_target(fixture.controller,world,1,unswbc::Position{7,5});
    REQUIRE(route);
    CHECK(route->target == unswbc::Position{5,7});
    fixture.controller.head.dragon_id = 1;
    auto const queen = sudo_win::Pathfinding{}.remembered_target(fixture.controller,world,1,unswbc::Position{7,5});
    REQUIRE(queen);
    CHECK(queen->target == unswbc::Position{7,5});
    fixture.controller.head.dragon_id = 6;
    auto const expired = sudo_win::Pathfinding{}.remembered_target(fixture.controller,world,6,unswbc::Position{7,5});
    REQUIRE(expired);
    CHECK(expired->target == unswbc::Position{7,5});
}

TEST_CASE("protected farms use observed countdowns and discard overdue predictions") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    fixture.tile({8,5}).pearl_time = 12;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto tiles = fixture.controller.vision.tiles;
    tiles.erase(std::remove_if(tiles.begin(),tiles.end(),
        [](auto const& tile) { return tile.get_position() == unswbc::Position{8,5}; }),tiles.end());
    fixture.controller.vision = unswbc::Vision{std::move(tiles)};
    auto const route = sudo_win::Pathfinding{}.remembered_target(fixture.controller,world,4);
    REQUIRE(route);
    CHECK(route->target == unswbc::Position{8,5});
    CHECK(route->pearl);
    auto const stale = sudo_win::Pathfinding{}.remembered_target(fixture.controller,world,20);
    REQUIRE(stale);
    CHECK_FALSE(stale->pearl);
}

TEST_CASE("starved portal approaches price destination income and reject exhausted pockets") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    fixture.controller.length = 3;
    fixture.controller.unit_count = 3;
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    fixture.tile({7,5}).get_edge(unswbc::Direction::EAST)
        = unswbc::Edge{false,unswbc::EdgeType::PORTAL,19};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    SECTION("small helper approaches a remote unexplored entrance") {
        auto const route = sudo_win::Pathfinding{}.portal_income_route(fixture.controller,world,30,false);
        REQUIRE(route);
        CHECK(route->target == unswbc::Position{7,5});
        CHECK(route->first_direction == unswbc::Direction::EAST);
        CHECK(route->distance == 3);
        CHECK(route->portal);
        CHECK_FALSE(sudo_win::Pathfinding{}.portal_income_route(fixture.controller,world,30,true));
    }
    SECTION("protected approach needs a productive mapped destination") {
        world.receive_report({sudo_win::MessageType::portal,25,6,0,0,39},25);
        world.receive_report({sudo_win::MessageType::empty,25,6,0,0,1043},25);
        CHECK(sudo_win::Pathfinding{}.portal_income_route(fixture.controller,world,30,true));
        CHECK_FALSE(sudo_win::Pathfinding{}.portal_income_route(fixture.controller,world,32,true));
        fixture.controller.length = 13;
        CHECK_FALSE(sudo_win::Pathfinding{}.portal_income_route(fixture.controller,world,30,true));
    }
    SECTION("remembered productive exits need onward room and actual income") {
        auto tiles = std::vector<unswbc::Tile>{};
        for (auto y = 0; y < 10; ++y) {
            for (auto x = 0; x < 10; ++x) {
                tiles.emplace_back(unswbc::Position{x,y}); tiles.back().pearl_time = -1;
            }
        }
        fixture.controller.vision = unswbc::Vision{std::move(tiles)};
        fixture.tile({7,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,19};
        fixture.tile({0,0}).get_edge(unswbc::Direction::WEST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,19};
        fixture.game.round_num = 30;
        fixture.tile({1,0}).pearl = true;
        world.update(fixture.controller,fixture.game);
        CHECK(sudo_win::Pathfinding{}.portal_income_route(fixture.controller,world,30,true));
        fixture.tile({1,0}).pearl = false;
        world.update(fixture.controller,fixture.game);
        CHECK_FALSE(sudo_win::Pathfinding{}.portal_income_route(fixture.controller,world,30,false));
        fixture.tile({0,0}).pearl = true;
        for (auto const d : {unswbc::Direction::NORTH,unswbc::Direction::EAST,unswbc::Direction::SOUTH}) {
            fixture.tile({0,0}).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        }
        world.update(fixture.controller,fixture.game);
        CHECK_FALSE(sudo_win::Pathfinding{}.portal_income_route(fixture.controller,world,30,false));
    }
    SECTION("sole survivor never makes a blind exploration investment") {
        fixture.controller.unit_count = 1;
        CHECK_FALSE(sudo_win::Pathfinding{}.portal_income_route(fixture.controller,world,30,false));
    }
}

TEST_CASE("route progress measures complete paths and respects directed portal edges") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto distances = sudo_win::Pathfinding{}.target_distances(fixture.controller,world,{7,5});
    CHECK(distances[55] == 2);
    CHECK(distances[56] == 1);
    CHECK(distances[54] == 3); // Returning west after an east step is not progress.
    fixture.tile({6,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::KELP};
    world.update(fixture.controller,fixture.game);
    distances = sudo_win::Pathfinding{}.target_distances(fixture.controller,world,{7,5});
    CHECK(distances[55] == 4);
    fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    fixture.tile({7,5}).get_edge(unswbc::Direction::WEST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    world.update(fixture.controller,fixture.game);
    distances = sudo_win::Pathfinding{}.target_distances(fixture.controller,world,{7,5});
    CHECK(distances[55] == 1);
    auto const approach = sudo_win::Pathfinding{}.target_distances(fixture.controller,world,{7,5},true);
    CHECK(approach[55] > 1);
}

TEST_CASE("fresh large champion claims override sender ordering without claiming certainty") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    fixture.controller.length = 3;
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    fixture.tile({7,5}).pearl = true;
    fixture.tile({5,7}).pearl = true;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    world.receive_report({sudo_win::MessageType::heartbeat,0,8,0,0,20},0);
    world.receive_report({sudo_win::MessageType::feeder,0,8,7,5,1},0);
    auto const route = sudo_win::Pathfinding{}.remembered_target(fixture.controller,world,0,unswbc::Position{7,5});
    REQUIRE(route);
    CHECK(route->target == unswbc::Position{5,7});
    auto const expired = sudo_win::Pathfinding{}.remembered_target(fixture.controller,world,3,unswbc::Position{7,5});
    REQUIRE(expired);
    CHECK(expired->target == unswbc::Position{7,5});
}

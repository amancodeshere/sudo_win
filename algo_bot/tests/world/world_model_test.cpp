#include "sudo_win/world/world_model.h"
#include "sudo_win/safety/safety.h"
#include "sudo_win/planner/simulation.h"
#include "sudo_win/pathfinding/pathfinding.h"
#include <algorithm>

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("world model observations") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game.round_num = 12;

    auto& east = fixture.tile({6, 5});
    east.pearl = true;
    east.pearl_time = 4;
    east.get_edge(unswbc::Direction::EAST) = unswbc::Edge{false, unswbc::EdgeType::PORTAL, 9};

    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);

    SECTION("visible cell state is remembered") {
        auto const& remembered = world.cell({6, 5});
        CHECK(remembered.seen);
        CHECK(remembered.last_seen_round == 12);
        CHECK(remembered.has_pearl);
        CHECK(remembered.pearl_time == 4);
    }

    SECTION("unique portal endpoints are remembered") {
        auto const* endpoints = world.portal_endpoints(9);
        REQUIRE(endpoints != nullptr);
        CHECK(endpoints->size() == 1);
    }

    SECTION("vision boundaries are exploration frontiers") {
        CHECK(world.unseen_neighbour_count({2, 2}) == 2);
    }
}

TEST_CASE("paired portal boundaries preserve heading and deduplicate both sides") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto const position : {unswbc::Position{5, 5}, unswbc::Position{7, 7}}) {
        fixture.tile(position).get_edge(unswbc::Direction::EAST)
            = unswbc::Edge{false, unswbc::EdgeType::PORTAL, 9};
        fixture.tile(position.add_dir(unswbc::Direction::EAST)).get_edge(unswbc::Direction::WEST)
            = unswbc::Edge{false, unswbc::EdgeType::PORTAL, 9};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    REQUIRE(world.portal_endpoints(9));
    CHECK(world.portal_endpoints(9)->size() == 2);
    CHECK(world.transition({5, 5}, unswbc::Direction::EAST) == unswbc::Position{8, 7});
    CHECK(world.transition({6, 5}, unswbc::Direction::WEST) == unswbc::Position{7, 7});
    CHECK(world.transition({7, 7}, unswbc::Direction::EAST) == unswbc::Position{6, 5});
    CHECK(world.transition({8, 7}, unswbc::Direction::WEST) == unswbc::Position{5, 5});
    auto safety = sudo_win::Safety{};
    auto simulation = sudo_win::Simulation{};

    SECTION("a visible empty exit can be crossed and scored as the actual destination") {
        CHECK(safety.is_safe_standard_move(fixture.controller, unswbc::Direction::EAST, &world));
        fixture.tile({8, 7}).pearl = true;
        auto const next = simulation.advance(fixture.controller, simulation.initial_state(fixture.controller, &world),
                                              unswbc::Direction::EAST, false, &world);
        REQUIRE(next);
        CHECK(next->body.front() == unswbc::Position{8, 7});
        CHECK(next->pearls == 1);
        CHECK(sudo_win::Pathfinding{}.visible_pearl_distance(fixture.controller, {5, 5}, &world) == 1);
    }
    SECTION("occupied exits remain fatal") {
        fixture.tile({8, 7}).dragon_part = unswbc::DragonPart{
            {8, 7}, 2, unswbc::Team::B, unswbc::Direction::WEST, false};
        CHECK(safety.standard_move_reason(fixture.controller, unswbc::Direction::EAST, &world)
              == sudo_win::SafetyReason::occupied);
    }
    SECTION("remembered occupancy does not certify an unseen exit") {
        auto tiles = fixture.controller.vision.tiles;
        tiles.erase(std::remove_if(tiles.begin(), tiles.end(), [](auto const& tile) {
            return tile.get_position() == unswbc::Position{8, 7};
        }), tiles.end());
        fixture.controller.vision = unswbc::Vision{std::move(tiles)};
        CHECK(safety.standard_move_reason(fixture.controller, unswbc::Direction::EAST, &world)
              == sudo_win::SafetyReason::unknown_tile);
    }
    SECTION("body reconstruction follows portal links rather than adjacent coordinates") {
        fixture.controller.head.position = {8, 7};
        fixture.tile({5, 5}).dragon_part = unswbc::DragonPart{
            {5, 5}, 0, unswbc::Team::A, unswbc::Direction::EAST, false};
        fixture.tile({5, 6}).dragon_part = unswbc::DragonPart{
            {5, 6}, 0, unswbc::Team::A, unswbc::Direction::NORTH, false};
        auto const state = simulation.initial_state(fixture.controller, &world);
        CHECK(state.body[1] == unswbc::Position{5, 5});
        CHECK(state.body[2] == unswbc::Position{5, 6});
        CHECK_FALSE(simulation.advance(fixture.controller, state, unswbc::Direction::WEST, false, &world));
    }
}

TEST_CASE("horizontal portal crossings choose the heading side of the partner") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto const position : {unswbc::Position{5, 5}, unswbc::Position{7, 7}}) {
        fixture.tile(position).get_edge(unswbc::Direction::NORTH)
            = unswbc::Edge{true, unswbc::EdgeType::PORTAL, 9};
        fixture.tile(position.add_dir(unswbc::Direction::NORTH)).get_edge(unswbc::Direction::SOUTH)
            = unswbc::Edge{true, unswbc::EdgeType::PORTAL, 9};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    CHECK(world.transition({5, 5}, unswbc::Direction::NORTH) == unswbc::Position{7, 6});
    CHECK(world.transition({5, 4}, unswbc::Direction::SOUTH) == unswbc::Position{7, 7});
}

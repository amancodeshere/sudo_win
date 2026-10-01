#include "sudo_win/planner/simulation.h"
#include "sudo_win/world/world_model.h"
#include "../engine_fixture.h"
#include <catch2/catch.hpp>

TEST_CASE("body simulation follows collision growth and sprint ordering") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto const position : {unswbc::Position{5, 6}, unswbc::Position{5, 7}}) {
        fixture.controller.get_tile(position)->dragon_part = unswbc::DragonPart{
            position, 0, unswbc::Team::A, unswbc::Direction::NORTH, false};
    }
    auto simulation = sudo_win::Simulation{};
    auto const initial = simulation.initial_state(fixture.controller);
    REQUIRE(initial.body.size() == 3);
    REQUIRE(initial.body.back() == unswbc::Position{5, 7});

    SECTION("body including the tail is blocked before advancement") {
        CHECK_FALSE(simulation.advance(fixture.controller, initial, unswbc::Direction::SOUTH));
        auto at_tail = initial;
        at_tail.body.front() = {6, 7};
        CHECK_FALSE(simulation.advance(fixture.controller, at_tail, unswbc::Direction::WEST));
    }
    SECTION("released segments become traversable on later moves") {
        auto east = simulation.advance(fixture.controller, initial, unswbc::Direction::EAST);
        REQUIRE(east);
        auto south = simulation.advance(fixture.controller, *east, unswbc::Direction::SOUTH);
        REQUIRE(south);
        auto west = simulation.advance(fixture.controller, *south, unswbc::Direction::WEST);
        REQUIRE(west);
        CHECK(west->body.front() == unswbc::Position{5, 6});
    }
    SECTION("collection holds the tail and costs are charged after collection") {
        fixture.controller.get_tile({6, 5})->pearl = true;
        auto east = simulation.advance(fixture.controller, initial, unswbc::Direction::EAST);
        REQUIRE(east);
        CHECK(east->body.size() == 4);
        CHECK(east->body.back() == initial.body.back());
        fixture.controller.get_tile({7, 5})->pearl = true;
        auto sprint = simulation.advance(fixture.controller, *east, unswbc::Direction::EAST, true);
        REQUIRE(sprint);
        CHECK(sprint->body.size() == 4);
        CHECK(sprint->pearls == 2);
    }
    SECTION("an extra step cannot be paid at minimum length") {
        auto east = simulation.advance(fixture.controller, initial, unswbc::Direction::EAST);
        REQUIRE(east);
        auto second = simulation.advance(fixture.controller, *east, unswbc::Direction::EAST, true);
        REQUIRE(second);
        CHECK(second->body.size() == 2);
        CHECK_FALSE(simulation.advance(fixture.controller, *second, unswbc::Direction::SOUTH, true));
    }
    SECTION("unranked visible body cannot be assumed to have vacated") {
        fixture.controller.get_tile({5, 6})->dragon_part.reset();
        auto partial = simulation.initial_state(fixture.controller);
        CHECK(partial.unranked_body.size() == 1);
        partial.body.front() = {6, 7};
        CHECK_FALSE(simulation.advance(fixture.controller, partial, unswbc::Direction::WEST));
    }
}

TEST_CASE("remembered mobility distinguishes closed terrain from unseen frontiers") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto simulation = sudo_win::Simulation{};
    auto const state = simulation.initial_state(fixture.controller, &world);
    CHECK(simulation.remembered_mobility(fixture.controller, state, world).open_frontier);
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        fixture.controller.get_tile({5, 5})->get_edge(direction)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
    }
    world.update(fixture.controller, fixture.game);
    auto const closed = simulation.remembered_mobility(fixture.controller, state, world);
    CHECK_FALSE(closed.open_frontier);
    CHECK(closed.area == 1);
}

TEST_CASE("simulation consumes a pearl only once on a looping route") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.length = 2;
    fixture.controller.get_tile({6, 5})->pearl = true;
    auto simulation = sudo_win::Simulation{};
    auto state = simulation.initial_state(fixture.controller);
    for (auto const direction : {unswbc::Direction::EAST, unswbc::Direction::SOUTH,
                                 unswbc::Direction::WEST, unswbc::Direction::NORTH,
                                 unswbc::Direction::EAST}) {
        auto next = simulation.advance(fixture.controller, state, direction);
        REQUIRE(next);
        state = *next;
    }
    CHECK(state.pearls == 1);
    CHECK(state.body.size() == 3);
}

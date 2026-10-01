#include "sudo_win/planner/planner.h"

#include "sudo_win/world/world_model.h"
#include "sudo_win/planner/simulation.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("baseline planner") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.get_tile({6, 5})->pearl = true;

    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto planner = sudo_win::Planner{};
    auto const action = planner.choose_action(fixture.controller,
                                              fixture.game,
                                              world,
                                              sudo_win::Role::collector);

    SECTION("returns a single movement action") {
        CHECK(action.kind == sudo_win::ActionKind::move);
        REQUIRE(action.steps.size() == 1);
    }

    SECTION("chooses an adjacent pearl") {
        REQUIRE(action.steps.size() == 1);
        CHECK(action.steps.front() == unswbc::Direction::EAST);
    }
}

TEST_CASE("planner validates and prices every step of short pearl sprints") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto& tile : fixture.controller.vision.tiles) {
        tile.pearl_time = -1;
    }
    for (auto x = 6; x <= 8; ++x) {
        fixture.controller.get_tile({x, 5})->pearl = true;
    }
    SECTION("an open pearl chain is collected with a profitable sprint") {
        auto world = sudo_win::WorldModel{fixture.game};
        world.update(fixture.controller, fixture.game);
        auto const action = sudo_win::Planner{}.choose_action(fixture.controller, fixture.game,
                                                              world, sudo_win::Role::collector);
        REQUIRE(action.kind == sudo_win::ActionKind::sprint);
        REQUIRE(action.steps.size() == 3);
        auto simulation = sudo_win::Simulation{};
        auto state = simulation.initial_state(fixture.controller, &world);
        for (std::size_t i = 0; i < action.steps.size(); ++i) {
            auto const next = simulation.advance(fixture.controller, state, action.steps[i], i > 0, &world);
            REQUIRE(next);
            state = *next;
        }
        CHECK(state.pearls == 3);
        CHECK(state.body.size() == 4);
    }
    SECTION("blocked intermediate edges prevent oversprinting through a pearl") {
        fixture.controller.get_tile({6, 5})->get_edge(unswbc::Direction::EAST)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
        auto world = sudo_win::WorldModel{fixture.game};
        world.update(fixture.controller, fixture.game);
        auto const action = sudo_win::Planner{}.choose_action(fixture.controller, fixture.game,
                                                              world, sudo_win::Role::collector);
        REQUIRE(action.steps.size() == 1);
        CHECK(action.steps.front() == unswbc::Direction::EAST);
    }
    SECTION("minimum length without pearls cannot afford additional steps") {
        fixture.controller.length = 2;
        for (auto& tile : fixture.controller.vision.tiles) {
            tile.pearl = false;
        }
        auto world = sudo_win::WorldModel{fixture.game};
        world.update(fixture.controller, fixture.game);
        auto const action = sudo_win::Planner{}.choose_action(fixture.controller, fixture.game,
                                                              world, sudo_win::Role::collector);
        CHECK(action.steps.size() == 1);
    }
}

TEST_CASE("an uncontested escape outranks a pearl threatened by a later enemy") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.get_tile({6, 5})->pearl = true;
    fixture.controller.get_tile({7, 5})->dragon_part = unswbc::DragonPart{
        {7, 5}, 4, unswbc::Team::B, unswbc::Direction::WEST, true};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto const action = sudo_win::Planner{}.choose_action(fixture.controller, fixture.game,
                                                          world, sudo_win::Role::collector);
    REQUIRE(action.steps.size() == 1);
    CHECK(action.steps.front() != unswbc::Direction::EAST);
}

TEST_CASE("planner prefers an escape over a pearl in a closed pocket") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.get_tile({6, 5})->pearl = true;
    for (auto const direction : {unswbc::Direction::NORTH, unswbc::Direction::EAST,
                                 unswbc::Direction::SOUTH}) {
        fixture.controller.get_tile({6, 5})->get_edge(direction)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto const action = sudo_win::Planner{}.choose_action(fixture.controller, fixture.game,
                                                          world, sudo_win::Role::collector);
    REQUIRE(action.steps.size() == 1);
    CHECK(action.steps.front() != unswbc::Direction::EAST);
}

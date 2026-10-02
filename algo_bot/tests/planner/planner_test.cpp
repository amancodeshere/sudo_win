#include "sudo_win/planner/planner.h"

#include "sudo_win/world/world_model.h"
#include "sudo_win/planner/simulation.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("baseline planner") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.tile({6, 5}).pearl = true;

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

TEST_CASE("a long dragon avoids a closed pocket beyond the search horizon") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.length = 20;
    auto const pocket = std::vector<unswbc::Position>{
        {6, 5}, {6, 4}, {6, 3}, {7, 3}, {7, 4}, {7, 5},
        {7, 6}, {6, 6}, {6, 7}, {7, 7}, {8, 7}};
    for (auto const p : pocket) {
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const neighbour = p.add_dir(direction);
            if (std::find(pocket.begin(), pocket.end(), neighbour) != pocket.end()
                || (p == unswbc::Position{6, 5} && neighbour == fixture.controller.get_position())) {
                continue;
            }
            fixture.tile(p).get_edge(direction)
                = unswbc::Edge{false, unswbc::EdgeType::KELP};
            if (auto* tile = fixture.controller.get_tile(neighbour)) {
                tile->get_edge(direction.get_opposite()) = unswbc::Edge{false, unswbc::EdgeType::KELP};
            }
        }
    }
    fixture.tile({6, 5}).pearl = true;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto const action = sudo_win::Planner{}.choose_action(fixture.controller, fixture.game,
                                                          world, sudo_win::Role::collector);
    REQUIRE(!action.steps.empty());
    CHECK(action.steps.front() != unswbc::Direction::EAST);
}

TEST_CASE("planner validates and prices every step of short pearl sprints") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto& tile : fixture.controller.vision.tiles) {
        tile.pearl_time = -1;
    }
    for (auto x = 6; x <= 8; ++x) {
        fixture.tile({x, 5}).pearl = true;
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
        fixture.tile({6, 5}).get_edge(unswbc::Direction::EAST)
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
    fixture.tile({6, 5}).pearl = true;
    fixture.tile({7, 5}).dragon_part = unswbc::DragonPart{
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
    fixture.tile({6, 5}).pearl = true;
    for (auto const direction : {unswbc::Direction::NORTH, unswbc::Direction::EAST,
                                 unswbc::Direction::SOUTH}) {
        fixture.tile({6, 5}).get_edge(direction)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto const action = sudo_win::Planner{}.choose_action(fixture.controller, fixture.game,
                                                          world, sudo_win::Role::collector);
    REQUIRE(action.steps.size() == 1);
    CHECK(action.steps.front() != unswbc::Direction::EAST);
}

TEST_CASE("a funded two step attack outweighs an adjacent pearl") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.tile({6,5}).pearl = true;
    fixture.tile({8,5}).dragon_part = unswbc::DragonPart{
        {8,5},4,unswbc::Team::B,unswbc::Direction::WEST,true};
    fixture.tile({8,6}).dragon_part = unswbc::DragonPart{
        {8,6},4,unswbc::Team::B,unswbc::Direction::NORTH,false};
    fixture.tile({8,7}).dragon_part = unswbc::DragonPart{
        {8,7},4,unswbc::Team::B,unswbc::Direction::NORTH,false};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto const action = sudo_win::Planner{false, false, false, true}.choose_action(fixture.controller, fixture.game,
                                                             world, sudo_win::Role::champion);
    REQUIRE(action.steps.size() == 1);
    CHECK(action.steps.front() != unswbc::Direction::EAST);
}

TEST_CASE("a shortening sprint escapes a loop that defeats ordinary movement") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.length = 4;
    for (auto& tile : fixture.controller.vision.tiles) {
        tile.pearl = false;
        tile.pearl_time = -1;
    }
    for (auto const p : std::vector<unswbc::Position>{{4,5},{3,5},{2,5}}) {
        fixture.tile(p).dragon_part = unswbc::DragonPart{
            p,0,unswbc::Team::A,p.x == 2 && p.y == 6 ? unswbc::Direction::NORTH : unswbc::Direction::EAST,false};
    }
    auto const loop = std::vector<unswbc::Position>{{5,5},{6,5},{6,4},{5,4}};
    for (auto const p : loop) {
        for (auto const d : unswbc::Direction::get_direction_list()) {
            auto const target = p.add_dir(d);
            if (std::find(loop.begin(),loop.end(),target) == loop.end()
                && !(p == unswbc::Position{5,5} && d == unswbc::Direction::WEST)) {
                fixture.tile(p).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
            }
        }
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto const action = sudo_win::Planner{}.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::champion);
    REQUIRE(action.kind == sudo_win::ActionKind::sprint);
    REQUIRE(action.steps.size() == 2);
    auto simulation = sudo_win::Simulation{};
    auto state = simulation.initial_state(fixture.controller,&world);
    for (std::size_t i = 0; i < action.steps.size(); ++i) {
        auto next = simulation.advance(fixture.controller,state,action.steps[i],i > 0,&world);
        REQUIRE(next);
        state = *next;
    }
    CHECK(state.body.size() == 3);
    auto budget = 512;
    CHECK(simulation.survival_depth(fixture.controller,state,6,budget,&world) == 6);
}

TEST_CASE("long snakes exploit five free steps with bounded profitable routes") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.length = 17;
    for (auto& tile : fixture.controller.vision.tiles) {
        tile.pearl_time = -1;
        tile.pearl = false;
    }
    for (auto const p : std::vector<unswbc::Position>{{6,5},{7,5},{8,5},{8,4},{8,3}}) {
        fixture.tile(p).pearl = true;
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto const action = sudo_win::Planner{}.choose_action(fixture.controller, fixture.game, world, sudo_win::Role::queen);
    REQUIRE(action.kind == sudo_win::ActionKind::sprint);
    REQUIRE(action.steps.size() == 5);
    auto simulation = sudo_win::Simulation{};
    auto state = simulation.initial_state(fixture.controller, &world);
    for (std::size_t i = 0; i < action.steps.size(); ++i) {
        auto next = simulation.advance(fixture.controller, state, action.steps[i], i > 0, &world);
        REQUIRE(next);
        state = *next;
    }
    CHECK(state.body.size() == 22);
    CHECK(state.pearls == 5);
}

TEST_CASE("queens and last survivors avoid funded later attacks without an experimental flag") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.tile({6,5}).pearl = true;
    for (auto const p : std::vector<unswbc::Position>{{8,5},{8,6},{8,7},{8,8},{7,8}}) {
        fixture.tile(p).dragon_part = unswbc::DragonPart{p,7,unswbc::Team::B,unswbc::Direction::NORTH,p == unswbc::Position{8,5}};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto const action = sudo_win::Planner{false}.choose_action(fixture.controller, fixture.game, world, sudo_win::Role::queen);
    REQUIRE(action.steps.size() == 1);
    CHECK(action.steps.front() != unswbc::Direction::EAST);
    fixture.controller.head.dragon_id = 9;
    auto const threats = sudo_win::Combat{}.threats(fixture.controller, &world);
    CHECK(threats[56].later_affordable_steps == 0);
    CHECK(threats[56].earlier_affordable_steps == 2);
}

TEST_CASE("a helper yields the queen's only escape despite an adjacent pearl") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 7;
    fixture.controller.length = 2;
    fixture.controller.unit_count = 2;
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    fixture.tile({6,5}).pearl = true;
    fixture.tile({6,4}).dragon_part = unswbc::DragonPart{
        {6,4},0,unswbc::Team::A,unswbc::Direction::SOUTH,true};
    fixture.tile({6,3}).dragon_part = unswbc::DragonPart{
        {6,3},0,unswbc::Team::A,unswbc::Direction::SOUTH,false};
    for (auto const direction : {unswbc::Direction::WEST,unswbc::Direction::EAST}) {
        fixture.tile({6,4}).get_edge(direction) = unswbc::Edge{false,unswbc::EdgeType::KELP};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto const action = sudo_win::Planner{false}.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::collector);
    REQUIRE(action.steps.size() == 1);
    CHECK(action.steps.front() != unswbc::Direction::EAST);
}

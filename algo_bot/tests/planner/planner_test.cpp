#include "sudo_win/planner/planner.h"

#include "sudo_win/world/world_model.h"

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

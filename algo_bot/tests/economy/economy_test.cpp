#include "sudo_win/economy/economy.h"

#include "sudo_win/config/config.h"
#include "sudo_win/pathfinding/pathfinding.h"
#include "sudo_win/world/world_model.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("economy destination scoring") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto const destination = unswbc::Position{6, 5};
    fixture.controller.get_tile(destination)->pearl = true;
    fixture.controller.get_tile(destination)->pearl_time = 3;

    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto economy = sudo_win::Economy{};
    auto pathfinding = sudo_win::Pathfinding{};

    SECTION("immediate pearls receive dominant positive value") {
        auto const score = economy.score_destination(fixture.controller, world, pathfinding, destination);
        CHECK(score >= sudo_win::config::score_immediate_pearl);
    }
}

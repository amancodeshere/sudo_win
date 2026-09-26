#include "sudo_win/economy/economy.h"

#include "sudo_win/config/config.h"
#include "sudo_win/pathfinding/pathfinding.h"
#include "sudo_win/world/world_model.h"

#include "../engine_fixture.h"
#include "../test_support.h"

namespace sudo_win::test {

auto run_economy_tests(TestSuite& suite) -> void {
    auto fixture = EngineFixture{};
    auto const destination = unswbc::Position{6, 5};
    fixture.controller.get_tile(destination)->pearl = true;
    fixture.controller.get_tile(destination)->pearl_time = 3;

    auto world = WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto economy = Economy{};
    auto pathfinding = Pathfinding{};

    auto const score = economy.score_destination(fixture.controller, world, pathfinding, destination);
    suite.expect(score >= config::score_immediate_pearl,
                 "economy gives immediate pearls dominant positive value");
}

} // namespace sudo_win::test

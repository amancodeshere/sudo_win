#include "sudo_win/planner/planner.h"

#include "sudo_win/world/world_model.h"

#include "../engine_fixture.h"
#include "../test_support.h"

namespace sudo_win::test {

auto run_planner_tests(TestSuite& suite) -> void {
    auto fixture = EngineFixture{};
    fixture.controller.get_tile({6, 5})->pearl = true;

    auto world = WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto planner = Planner{};
    auto const action = planner.choose_action(fixture.controller, fixture.game, world, Role::collector);

    suite.expect(action.kind == ActionKind::move, "baseline planner returns a move");
    suite.expect(action.steps.size() == 1 && action.steps.front() == unswbc::Direction::EAST,
                 "baseline planner chooses an adjacent pearl");
}

} // namespace sudo_win::test

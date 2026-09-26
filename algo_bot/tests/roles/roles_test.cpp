#include "sudo_win/roles/roles.h"

#include "../engine_fixture.h"
#include "../test_support.h"

namespace sudo_win::test {

auto run_roles_tests(TestSuite& suite) -> void {
    auto fixture = EngineFixture{};
    auto roles = RoleManager{};

    suite.expect(roles.choose_role(fixture.controller, fixture.game) == Role::champion,
                 "dragon zero begins as champion");

    fixture.controller.head.dragon_id = 1;
    suite.expect(roles.choose_role(fixture.controller, fixture.game) == Role::scout,
                 "non-champion IDs receive deterministic roles");
    suite.expect(roles.score_move(Role::scout, 0, 3, 0) > roles.score_move(Role::collector, 0, 3, 0),
                 "scouts value exploration frontiers more than collectors");
}

} // namespace sudo_win::test

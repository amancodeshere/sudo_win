#include "sudo_win/splitting/splitting.h"

#include "../engine_fixture.h"
#include "../test_support.h"

namespace sudo_win::test {

auto run_splitting_tests(TestSuite& suite) -> void {
    auto fixture = EngineFixture{};
    fixture.controller.length = 20;
    auto policy = SplittingPolicy{};

    suite.expect(!policy.consider(fixture.controller, fixture.game, Role::collector, 49).has_value(),
                 "splitting remains disabled in the stable baseline");
}

} // namespace sudo_win::test

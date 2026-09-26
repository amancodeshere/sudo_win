#include "sudo_win/safety/safety.h"

#include "../engine_fixture.h"
#include "../test_support.h"

namespace sudo_win::test {

auto run_safety_tests(TestSuite& suite) -> void {
    auto fixture = EngineFixture{};
    auto safety = Safety{};

    suite.expect(safety.safe_standard_moves(fixture.controller).size() == 4,
                 "all four empty adjacent tiles are safe");

    auto* origin = fixture.controller.get_tile({5, 5});
    origin->get_edge(unswbc::Direction::NORTH) = unswbc::Edge{true, unswbc::EdgeType::KELP};
    suite.expect(!safety.is_safe_standard_move(fixture.controller, unswbc::Direction::NORTH),
                 "kelp is rejected");

    origin->get_edge(unswbc::Direction::SOUTH) = unswbc::Edge{true, unswbc::EdgeType::PORTAL, 7};
    suite.expect(!safety.is_safe_standard_move(fixture.controller, unswbc::Direction::SOUTH),
                 "unknown portal exits are rejected");

    auto* east = fixture.controller.get_tile({6, 5});
    east->dragon_part = unswbc::DragonPart{{6, 5}, 1, unswbc::Team::B, unswbc::Direction::WEST, false};
    suite.expect(!safety.is_safe_standard_move(fixture.controller, unswbc::Direction::EAST),
                 "occupied destinations are rejected");
}

} // namespace sudo_win::test

#include "sudo_win/world/world_model.h"

#include "../engine_fixture.h"
#include "../test_support.h"

namespace sudo_win::test {

auto run_world_model_tests(TestSuite& suite) -> void {
    auto fixture = EngineFixture{};
    fixture.game.round_num = 12;

    auto* east = fixture.controller.get_tile({6, 5});
    east->pearl = true;
    east->pearl_time = 4;
    east->get_edge(unswbc::Direction::EAST) = unswbc::Edge{false, unswbc::EdgeType::PORTAL, 9};

    auto world = WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto const& remembered = world.cell({6, 5});

    suite.expect(remembered.seen, "visible cells become known");
    suite.expect(remembered.last_seen_round == 12, "world model records observation round");
    suite.expect(remembered.has_pearl && remembered.pearl_time == 4,
                 "world model records pearl state");

    auto const* endpoints = world.portal_endpoints(9);
    suite.expect(endpoints != nullptr && endpoints->size() == 1,
                 "world model records unique portal endpoints");
    suite.expect(world.unseen_neighbour_count({2, 2}) == 2,
                 "world model identifies frontiers at the vision boundary");
}

} // namespace sudo_win::test

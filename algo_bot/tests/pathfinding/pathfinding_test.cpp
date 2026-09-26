#include "sudo_win/pathfinding/pathfinding.h"

#include "../engine_fixture.h"
#include "../test_support.h"

namespace sudo_win::test {

auto run_pathfinding_tests(TestSuite& suite) -> void {
    auto fixture = EngineFixture{};
    auto pathfinding = Pathfinding{};

    suite.expect(pathfinding.visible_reachable_area(fixture.controller, {6, 5}) == 49,
                 "flood fill reaches the entire empty vision window");

    fixture.controller.get_tile({7, 5})->pearl = true;
    suite.expect(pathfinding.visible_pearl_distance(fixture.controller, {6, 5}) == 1,
                 "BFS finds the nearest visible pearl");

    fixture.controller.get_tile({6, 5})->get_edge(unswbc::Direction::EAST)
        = unswbc::Edge{false, unswbc::EdgeType::KELP};
    fixture.controller.get_tile({7, 5})->get_edge(unswbc::Direction::WEST)
        = unswbc::Edge{false, unswbc::EdgeType::KELP};
    suite.expect(pathfinding.visible_pearl_distance(fixture.controller, {6, 5}) == 3,
                 "BFS routes around a blocked direct edge");
}

} // namespace sudo_win::test

#include "test_support.h"

namespace sudo_win::test {

auto run_combat_tests(TestSuite& suite) -> void;
auto run_economy_tests(TestSuite& suite) -> void;
auto run_endgame_tests(TestSuite& suite) -> void;
auto run_geometry_tests(TestSuite& suite) -> void;
auto run_pathfinding_tests(TestSuite& suite) -> void;
auto run_planner_tests(TestSuite& suite) -> void;
auto run_roles_tests(TestSuite& suite) -> void;
auto run_safety_tests(TestSuite& suite) -> void;
auto run_sonar_tests(TestSuite& suite) -> void;
auto run_splitting_tests(TestSuite& suite) -> void;
auto run_world_model_tests(TestSuite& suite) -> void;

} // namespace sudo_win::test

auto main() -> int {
    auto suite = sudo_win::test::TestSuite{};
    sudo_win::test::run_combat_tests(suite);
    sudo_win::test::run_economy_tests(suite);
    sudo_win::test::run_endgame_tests(suite);
    sudo_win::test::run_geometry_tests(suite);
    sudo_win::test::run_pathfinding_tests(suite);
    sudo_win::test::run_planner_tests(suite);
    sudo_win::test::run_roles_tests(suite);
    sudo_win::test::run_safety_tests(suite);
    sudo_win::test::run_sonar_tests(suite);
    sudo_win::test::run_splitting_tests(suite);
    sudo_win::test::run_world_model_tests(suite);
    return suite.finish();
}

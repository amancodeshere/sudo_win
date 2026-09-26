#include "sudo_win/geometry/geometry.h"

#include "../test_support.h"

namespace sudo_win::test {

auto run_geometry_tests(TestSuite& suite) -> void {
    suite.expect(geometry::wrap(-1, 10) == 9, "wrap handles negative coordinates");
    suite.expect(geometry::wrap(10, 10) == 0, "wrap handles upper boundary");
    suite.expect(geometry::toroidal_axis_distance(0, 9, 10) == 1, "axis distance crosses boundary");
    suite.expect(geometry::toroidal_manhattan({0, 0}, {9, 9}, 10, 10) == 2,
                 "Manhattan distance uses toroidal topology");
    suite.expect(geometry::toroidal_chebyshev({0, 0}, {9, 8}, 10, 10) == 2,
                 "Chebyshev distance uses toroidal topology");
    suite.expect(geometry::direction_index(unswbc::Direction::WEST) == 3,
                 "direction indices follow engine edge order");
}

} // namespace sudo_win::test

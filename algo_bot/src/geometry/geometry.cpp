#include "../../include/sudo_win/geometry/geometry.h"

#include <algorithm>
#include <cstdlib>

namespace sudo_win::geometry {

auto direction_index(unswbc::Direction direction) -> std::size_t {
    switch (direction.value) {
    case unswbc::Direction::NORTH: return 0;
    case unswbc::Direction::EAST: return 1;
    case unswbc::Direction::SOUTH: return 2;
    default: return 3;
    }
}

auto wrap(int value, int size) -> int {
    return (value % size + size) % size;
}

auto toroidal_axis_distance(int first, int second, int size) -> int {
    auto const direct = std::abs(first - second);
    return std::min(direct, size - direct);
}

auto toroidal_manhattan(unswbc::Position first, unswbc::Position second, int width, int height) -> int {
    return toroidal_axis_distance(first.x, second.x, width)
         + toroidal_axis_distance(first.y, second.y, height);
}

auto toroidal_chebyshev(unswbc::Position first, unswbc::Position second, int width, int height) -> int {
    return std::max(toroidal_axis_distance(first.x, second.x, width),
                    toroidal_axis_distance(first.y, second.y, height));
}

} // namespace sudo_win::geometry

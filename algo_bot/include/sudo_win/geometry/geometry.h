#ifndef SUDO_WIN_GEOMETRY_GEOMETRY_H
#define SUDO_WIN_GEOMETRY_GEOMETRY_H

#include "../engine/helper.h"

#include <cstddef>

namespace sudo_win::geometry {

[[nodiscard]] auto direction_index(unswbc::Direction direction) -> std::size_t;
[[nodiscard]] auto wrap(int value, int size) -> int;
[[nodiscard]] auto toroidal_axis_distance(int first, int second, int size) -> int;
[[nodiscard]] auto toroidal_manhattan(unswbc::Position first,
                                      unswbc::Position second,
                                      int width,
                                      int height) -> int;
[[nodiscard]] auto toroidal_chebyshev(unswbc::Position first,
                                      unswbc::Position second,
                                      int width,
                                      int height) -> int;

} // namespace sudo_win::geometry

#endif // SUDO_WIN_GEOMETRY_GEOMETRY_H

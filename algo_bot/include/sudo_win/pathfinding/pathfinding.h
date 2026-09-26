#ifndef SUDO_WIN_PATHFINDING_PATHFINDING_H
#define SUDO_WIN_PATHFINDING_PATHFINDING_H

#include "../engine/helper.h"

namespace sudo_win {

class Pathfinding {
public:
    [[nodiscard]] auto visible_reachable_area(unswbc::Controller const& controller,
                                              unswbc::Position start) const -> int;
    [[nodiscard]] auto visible_pearl_distance(unswbc::Controller const& controller,
                                              unswbc::Position start) const -> int;
};

} // namespace sudo_win

#endif // SUDO_WIN_PATHFINDING_PATHFINDING_H

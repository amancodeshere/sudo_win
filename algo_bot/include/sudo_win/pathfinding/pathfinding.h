#ifndef SUDO_WIN_PATHFINDING_PATHFINDING_H
#define SUDO_WIN_PATHFINDING_PATHFINDING_H

#include "../engine/helper.h"
#include <optional>

namespace sudo_win {

class WorldModel;

struct TargetRoute {
    unswbc::Position target;
    unswbc::Direction first_direction = unswbc::Direction::NORTH;
    int distance = 0;
    int value = 0;
    bool pearl = false;
    bool portal = false;
};

class Pathfinding {
public:
    [[nodiscard]] auto remembered_target(unswbc::Controller const& controller,
                                         WorldModel const& world,
                                         int round,
                                         std::optional<unswbc::Position> preferred = std::nullopt,
                                         bool protected_unit = false,
                                         bool portal_scout = false) const
        -> std::optional<TargetRoute>;
    [[nodiscard]] auto portal_income_route(unswbc::Controller const& controller,
                                           WorldModel const& world, int round,
                                           bool protected_unit) const -> std::optional<TargetRoute>;
    [[nodiscard]] auto visible_reachable_area(unswbc::Controller const& controller,
                                              unswbc::Position start,
                                              WorldModel const* world = nullptr) const -> int;
    [[nodiscard]] auto visible_pearl_distance(unswbc::Controller const& controller,
                                              unswbc::Position start,
                                              WorldModel const* world = nullptr) const -> int;
};

} // namespace sudo_win

#endif // SUDO_WIN_PATHFINDING_PATHFINDING_H

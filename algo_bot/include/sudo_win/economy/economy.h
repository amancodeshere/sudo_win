#ifndef SUDO_WIN_ECONOMY_ECONOMY_H
#define SUDO_WIN_ECONOMY_ECONOMY_H

#include "../engine/helper.h"
#include "../types/types.h"
#include <optional>

namespace sudo_win {

class Pathfinding;
class WorldModel;

class Economy {
public:
    [[nodiscard]] auto queen_donation(unswbc::Controller const& controller, unswbc::Game const& game,
                                      WorldModel const& world, Role role) const -> std::optional<PlannedAction>;
    [[nodiscard]] auto score_destination(unswbc::Controller const& controller,
                                         WorldModel const& world,
                                         Pathfinding const& pathfinding,
                                         unswbc::Position destination) const -> int;
};

} // namespace sudo_win

#endif // SUDO_WIN_ECONOMY_ECONOMY_H

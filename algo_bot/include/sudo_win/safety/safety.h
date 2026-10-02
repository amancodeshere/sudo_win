#ifndef SUDO_WIN_SAFETY_SAFETY_H
#define SUDO_WIN_SAFETY_SAFETY_H

#include "../engine/helper.h"

#include <vector>
#include <optional>

namespace sudo_win {

enum class SafetyReason { safe, wall, occupied, unknown_tile, unknown_portal };
class WorldModel;

class Safety {
public:
    [[nodiscard]] auto standard_move_reason(unswbc::Controller const& controller,
                                            unswbc::Direction direction,
                                            WorldModel const* world = nullptr) const -> SafetyReason;
    [[nodiscard]] auto is_safe_standard_move(unswbc::Controller const& controller,
                                             unswbc::Direction direction,
                                             WorldModel const* world = nullptr) const -> bool;
    [[nodiscard]] auto safe_standard_moves(unswbc::Controller const& controller,
                                           WorldModel const* world = nullptr) const
        -> std::vector<unswbc::Direction>;
    [[nodiscard]] auto least_bad_fallback(unswbc::Controller const& controller) const -> unswbc::Direction;
    // An uncertain escape, never certified as an ordinary safe move.
    [[nodiscard]] auto remembered_portal_escape(unswbc::Controller const& controller,
                                                WorldModel const& world, int round) const
        -> std::optional<unswbc::Direction>;
    [[nodiscard]] auto helper_portal_probe(unswbc::Controller const& controller,
                                          WorldModel const& world, int round) const
        -> std::optional<unswbc::Direction>;
    [[nodiscard]] auto surveyed_portal_route(unswbc::Controller const& controller,
                                            WorldModel const& world, int round) const
        -> std::optional<unswbc::Direction>;
};

} // namespace sudo_win

#endif // SUDO_WIN_SAFETY_SAFETY_H

#ifndef SUDO_WIN_SAFETY_SAFETY_H
#define SUDO_WIN_SAFETY_SAFETY_H

#include "../engine/helper.h"

#include <vector>

namespace sudo_win {

enum class SafetyReason { safe, wall, occupied, unknown_tile, unknown_portal };

class Safety {
public:
    [[nodiscard]] auto standard_move_reason(unswbc::Controller const& controller,
                                            unswbc::Direction direction) const -> SafetyReason;
    [[nodiscard]] auto is_safe_standard_move(unswbc::Controller const& controller,
                                             unswbc::Direction direction) const -> bool;
    [[nodiscard]] auto safe_standard_moves(unswbc::Controller const& controller) const
        -> std::vector<unswbc::Direction>;
    [[nodiscard]] auto least_bad_fallback(unswbc::Controller const& controller) const -> unswbc::Direction;
};

} // namespace sudo_win

#endif // SUDO_WIN_SAFETY_SAFETY_H

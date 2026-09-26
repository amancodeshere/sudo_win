#ifndef SUDO_WIN_SPLITTING_SPLITTING_H
#define SUDO_WIN_SPLITTING_SPLITTING_H

#include "../engine/helper.h"
#include "../types/types.h"

#include <optional>

namespace sudo_win {

class SplittingPolicy {
public:
    [[nodiscard]] auto consider(unswbc::Controller const& controller,
                                unswbc::Game const& game,
                                Role role,
                                int reachable_area) const -> std::optional<PlannedAction>;
};

} // namespace sudo_win

#endif // SUDO_WIN_SPLITTING_SPLITTING_H

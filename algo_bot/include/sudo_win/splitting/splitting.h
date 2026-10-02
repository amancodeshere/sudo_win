#ifndef SUDO_WIN_SPLITTING_SPLITTING_H
#define SUDO_WIN_SPLITTING_SPLITTING_H

#include "../engine/helper.h"
#include "../types/types.h"
#include "../config/config.h"

#include <optional>

namespace sudo_win {
class WorldModel;

class SplittingPolicy {
public:
    [[nodiscard]] auto growth_rejection() const -> std::string_view { return growth_rejection_; }
    [[nodiscard]] auto grow_population(unswbc::Controller const& controller,
                                       unswbc::Game const& game,
                                       Role role, WorldModel const& world) const
        -> std::optional<PlannedAction>;
    [[nodiscard]] auto rescue(unswbc::Controller const& controller,
                              WorldModel const& world,
                              bool certainly_trapped,
                              bool preserve_parent = false) const -> std::optional<PlannedAction>;
    [[nodiscard]] auto consider(unswbc::Controller const& controller,
                                unswbc::Game const& game,
                                Role role,
                                int reachable_area,
                                WorldModel const* world = nullptr,
                                bool enabled = config::enable_splitting) const -> std::optional<PlannedAction>;
private:
    mutable std::string_view growth_rejection_ = "not evaluated";
};

} // namespace sudo_win

#endif // SUDO_WIN_SPLITTING_SPLITTING_H

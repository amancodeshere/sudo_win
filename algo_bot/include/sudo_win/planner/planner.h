#ifndef SUDO_WIN_PLANNER_PLANNER_H
#define SUDO_WIN_PLANNER_PLANNER_H

#include "../combat/combat.h"
#include "../economy/economy.h"
#include "../endgame/endgame.h"
#include "../pathfinding/pathfinding.h"
#include "../roles/roles.h"
#include "../safety/safety.h"
#include "../splitting/splitting.h"
#include "../types/types.h"

namespace sudo_win {

class WorldModel;

class Planner {
public:
    explicit Planner(bool sprinting = config::enable_sprinting,
                     bool splitting = config::enable_splitting,
                     bool growth_splitting = config::enable_growth_splitting)
    : sprinting_{sprinting}, splitting_enabled_{splitting}, growth_splitting_{growth_splitting} {}
    [[nodiscard]] auto choose_action(unswbc::Controller const& controller,
                                     unswbc::Game const& game,
                                     WorldModel const& world,
                                     Role role) const -> PlannedAction;

private:
    bool sprinting_;
    bool splitting_enabled_;
    bool growth_splitting_;
    mutable std::optional<unswbc::Position> target_;
    mutable int target_round_ = -1;
    Safety safety_;
    Pathfinding pathfinding_;
    Economy economy_;
    RoleManager roles_;
    Combat combat_;
    SplittingPolicy splitting_;
    Endgame endgame_;
};

} // namespace sudo_win

#endif // SUDO_WIN_PLANNER_PLANNER_H

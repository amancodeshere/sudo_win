#include "../../include/sudo_win/roles/roles.h"

namespace sudo_win {

auto RoleManager::choose_role(unswbc::Controller const& controller, unswbc::Game const&) const -> Role {
    if (controller.get_id() == 0) {
        return Role::champion;
    }
    switch (controller.get_id() % 4) {
    case 0: return Role::collector;
    case 1: return Role::scout;
    case 2: return Role::blocker;
    default: return Role::hunter;
    }
}

auto RoleManager::score_move(Role role,
                             int reachable_area,
                             int frontier_count,
                             int combat_score) const -> int {
    switch (role) {
    case Role::champion: return reachable_area * 120 + combat_score;
    case Role::collector: return reachable_area * 40;
    case Role::scout: return frontier_count * 250;
    case Role::blocker: return combat_score / 4;
    case Role::hunter: return combat_score / 2;
    }
    return 0;
}

} // namespace sudo_win

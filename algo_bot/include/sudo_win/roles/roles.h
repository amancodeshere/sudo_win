#ifndef SUDO_WIN_ROLES_ROLES_H
#define SUDO_WIN_ROLES_ROLES_H

#include "../engine/helper.h"
#include "../types/types.h"

namespace sudo_win {

class RoleManager {
public:
    [[nodiscard]] auto choose_role(unswbc::Controller const& controller,
                                   unswbc::Game const& game) const -> Role;
    [[nodiscard]] auto score_move(Role role,
                                  int reachable_area,
                                  int frontier_count,
                                  int combat_score) const -> int;
};

} // namespace sudo_win

#endif // SUDO_WIN_ROLES_ROLES_H

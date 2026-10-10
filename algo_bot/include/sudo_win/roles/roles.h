#ifndef SUDO_WIN_ROLES_ROLES_H
#define SUDO_WIN_ROLES_ROLES_H

#include "../engine/helper.h"
#include "../types/types.h"
#include <unordered_map>

namespace sudo_win {
class WorldModel;

class RoleManager {
public:
    [[nodiscard]] auto choose_role(unswbc::Controller const& controller,
                                   unswbc::Game const& game, WorldModel const* world = nullptr) const -> Role;
    [[nodiscard]] auto score_move(Role role,
                                  int reachable_area,
                                  int frontier_count,
                                  int combat_score) const -> int;
private:
    struct LengthEstimate { int length; int round; };
    mutable std::unordered_map<int, LengthEstimate> allies_;
    mutable int champion_id_ = -1;
};

} // namespace sudo_win

#endif // SUDO_WIN_ROLES_ROLES_H

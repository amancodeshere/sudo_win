#include "../../include/sudo_win/combat/combat.h"

#include "../../include/sudo_win/config/config.h"

namespace sudo_win {

auto Combat::destination_risk(unswbc::Controller const& controller,
                              unswbc::Position destination,
                              Role role) const -> int {
    auto score = 0;
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        auto const* tile = controller.get_tile(destination.add_dir(direction));
        if (tile == nullptr) {
            continue;
        }
        auto const* part = tile->get_dragon();
        if (part == nullptr || !part->is_head() || part->get_team() == controller.get_team()) {
            continue;
        }

        score += config::score_enemy_head_risk;
        if (part->get_id() > controller.get_id()) {
            score += config::score_enemy_head_late_risk;
        }
    }

    if (role == Role::champion) {
        score *= config::score_champion_risk_multiplier;
    }
    return score;
}

} // namespace sudo_win

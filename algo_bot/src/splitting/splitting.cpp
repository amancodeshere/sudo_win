#include "../../include/sudo_win/splitting/splitting.h"

#include "../../include/sudo_win/config/config.h"

#include <algorithm>

namespace sudo_win {

auto SplittingPolicy::consider(unswbc::Controller const& controller,
                               unswbc::Game const& game,
                               Role role,
                               int reachable_area) const -> std::optional<PlannedAction> {
    if (!config::enable_splitting || role == Role::champion
        || game.get_round_num() >= config::endgame_start_round) {
        return std::nullopt;
    }
    if (controller.get_length() < config::split_min_length
        || controller.get_unit_count() >= std::min(config::soft_unit_cap, game.get_unit_limit())
        || reachable_area < 16) {
        return std::nullopt;
    }

    auto const child_size = std::max(unswbc::Constants::MIN_SIZE, controller.get_length() / 3);
    if (!controller.can_split(child_size)) {
        return std::nullopt;
    }

    auto action = PlannedAction{};
    action.kind = ActionKind::split;
    action.split_size = child_size;
    action.score = 0;
    action.reason = "conservative split";
    return action;
}

} // namespace sudo_win

#include "../../include/sudo_win/endgame/endgame.h"

#include "../../include/sudo_win/config/config.h"
#include <algorithm>

namespace sudo_win {

auto Endgame::active(unswbc::Game const& game) const -> bool {
    return game.get_round_num() >= config::endgame_start_round;
}

auto Endgame::score_destination(unswbc::Controller const&,
                                unswbc::Game const& game,
                                Role role,
                                int reachable_area,
                                int combat_score) const -> int {
    if (game.get_round_num() < config::endgame_ramp_round) {
        return 0;
    }

    auto score = reachable_area * 250 + combat_score;
    if (role == Role::champion) {
        score += reachable_area * 250 + combat_score;
    }
    auto const ramp = config::endgame_start_round - config::endgame_ramp_round;
    auto const progress = std::min(ramp, game.get_round_num() - config::endgame_ramp_round + 1);
    return score * progress / ramp;
}

} // namespace sudo_win

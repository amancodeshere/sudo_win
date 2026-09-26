#include "../../include/sudo_win/endgame/endgame.h"

#include "../../include/sudo_win/config/config.h"

namespace sudo_win {

auto Endgame::active(unswbc::Game const& game) const -> bool {
    return game.get_round_num() >= config::endgame_start_round;
}

auto Endgame::score_destination(unswbc::Controller const&,
                                unswbc::Game const& game,
                                Role role,
                                int reachable_area,
                                int combat_score) const -> int {
    if (!active(game)) {
        return 0;
    }

    auto score = reachable_area * 250 + combat_score;
    if (role == Role::champion) {
        score += reachable_area * 250 + combat_score;
    }
    return score;
}

} // namespace sudo_win

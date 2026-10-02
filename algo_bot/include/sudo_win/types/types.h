#ifndef SUDO_WIN_TYPES_TYPES_H
#define SUDO_WIN_TYPES_TYPES_H

#include "../engine/helper.h"

#include <limits>
#include <optional>
#include <string_view>
#include <vector>

namespace sudo_win {

enum class ActionKind {
    move,
    sprint,
    split,
    donate,
};

enum class Role {
    queen,
    champion,
    collector,
    scout,
    blocker,
    hunter,
};

struct PlannedAction {
    ActionKind kind = ActionKind::move;
    std::vector<unswbc::Direction> steps;
    int split_size = 0;
    int recipient_id = -1;
    std::optional<unswbc::Position> resource_target;
    int resource_distance = 0;
    int score = std::numeric_limits<int>::min();
    std::string_view reason = "fallback";
};

struct MoveCandidate {
    explicit MoveCandidate(unswbc::Direction direction)
    : direction{direction} {}

    [[nodiscard]] auto total_score() const -> int {
        return mobility_score + economy_score + exploration_score + combat_score + role_score + endgame_score;
    }

    unswbc::Direction direction;
    int mobility_score = 0;
    int economy_score = 0;
    int exploration_score = 0;
    int combat_score = 0;
    int role_score = 0;
    int endgame_score = 0;
};

} // namespace sudo_win

#endif // SUDO_WIN_TYPES_TYPES_H

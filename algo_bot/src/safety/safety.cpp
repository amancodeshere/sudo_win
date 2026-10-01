#include "../../include/sudo_win/safety/safety.h"
#include "../../include/sudo_win/world/world_model.h"

namespace sudo_win {

auto Safety::is_safe_standard_move(unswbc::Controller const& controller,
                                   unswbc::Direction direction, WorldModel const* world) const -> bool {
    return standard_move_reason(controller, direction, world) == SafetyReason::safe;
}

auto Safety::standard_move_reason(unswbc::Controller const& controller,
                                  unswbc::Direction direction, WorldModel const* world) const -> SafetyReason {
    auto const* origin = controller.get_tile(controller.get_position());
    if (origin == nullptr) {
        return SafetyReason::unknown_tile;
    }

    auto const& edge = origin->get_edge(direction);
    if (!edge.is_passable()) {
        return SafetyReason::wall;
    }
    auto target = controller.get_position().add_dir(direction);
    if (edge.is_portal()) {
        auto const exit = world != nullptr ? world->transition(controller.get_position(), direction) : std::nullopt;
        if (!exit) {
            return SafetyReason::unknown_portal;
        }
        target = *exit;
    }

    auto const* destination = controller.get_tile(target);
    if (destination == nullptr) {
        return SafetyReason::unknown_tile;
    }
    return destination->get_dragon() == nullptr ? SafetyReason::safe : SafetyReason::occupied;
}

auto Safety::safe_standard_moves(unswbc::Controller const& controller, WorldModel const* world) const
    -> std::vector<unswbc::Direction> {
    auto moves = std::vector<unswbc::Direction>{};
    moves.reserve(4);
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        if (is_safe_standard_move(controller, direction, world)) {
            moves.push_back(direction);
        }
    }
    return moves;
}

auto Safety::least_bad_fallback(unswbc::Controller const& controller) const -> unswbc::Direction {
    auto best = controller.get_dir();
    auto best_rank = 5;
    // Prefer an uncertain escape over a certainly fatal wall/body collision.
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        auto const reason = standard_move_reason(controller, direction);
        auto const rank = reason == SafetyReason::safe ? 0
                        : reason == SafetyReason::unknown_tile ? 1
                        : reason == SafetyReason::unknown_portal ? 2
                        : reason == SafetyReason::occupied ? 3 : 4;
        if (rank < best_rank || (rank == best_rank && direction == controller.get_dir())) {
            best = direction;
            best_rank = rank;
        }
    }
    return best;
}

} // namespace sudo_win

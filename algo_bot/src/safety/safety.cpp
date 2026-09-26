#include "../../include/sudo_win/safety/safety.h"

namespace sudo_win {

auto Safety::is_safe_standard_move(unswbc::Controller const& controller,
                                   unswbc::Direction direction) const -> bool {
    auto const* origin = controller.get_tile(controller.get_position());
    if (origin == nullptr) {
        return false;
    }

    auto const& edge = origin->get_edge(direction);
    if (!edge.is_passable() || edge.is_portal()) {
        return false;
    }

    auto const* destination = controller.get_tile(controller.get_position().add_dir(direction));
    return destination != nullptr && destination->get_dragon() == nullptr;
}

auto Safety::safe_standard_moves(unswbc::Controller const& controller) const
    -> std::vector<unswbc::Direction> {
    auto moves = std::vector<unswbc::Direction>{};
    moves.reserve(4);
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        if (is_safe_standard_move(controller, direction)) {
            moves.push_back(direction);
        }
    }
    return moves;
}

auto Safety::least_bad_fallback(unswbc::Controller const& controller) const -> unswbc::Direction {
    auto const* origin = controller.get_tile(controller.get_position());
    if (origin != nullptr) {
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            if (origin->get_edge(direction).is_passable()) {
                return direction;
            }
        }
    }
    return unswbc::Direction::NORTH;
}

} // namespace sudo_win

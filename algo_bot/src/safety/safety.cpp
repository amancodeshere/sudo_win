#include "../../include/sudo_win/safety/safety.h"
#include "../../include/sudo_win/world/world_model.h"
#include "../../include/sudo_win/planner/simulation.h"
#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/geometry/geometry.h"

#include <algorithm>

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

auto Safety::remembered_portal_escape(unswbc::Controller const& controller,
                                       WorldModel const& world, int round) const
    -> std::optional<unswbc::Direction> {
    auto const* origin = controller.get_tile(controller.get_position());
    if (origin == nullptr) {
        return std::nullopt;
    }
    auto const state = Simulation{}.initial_state(controller, &world);
    if (!state.unranked_body.empty() || std::any_of(state.body.begin(), state.body.end(), [](auto p) {
        return p.x < 0 || p.y < 0;
    })) {
        return std::nullopt;
    }
    auto const blocked = [&](unswbc::Position p) {
        return std::find(state.body.begin(), state.body.end(), p) != state.body.end()
            || world.cell(p).occupant.has_value();
    };
    auto best = std::optional<unswbc::Direction>{};
    auto best_score = -1;
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        if (!origin->get_edge(direction).is_portal()) {
            continue;
        }
        auto const exit = world.transition(controller.get_position(), direction);
        if (!exit || controller.get_tile(*exit) != nullptr || !world.has_seen(*exit) || blocked(*exit)) {
            continue;
        }
        auto const age = round - world.cell(*exit).last_seen_round;
        if (age < 0 || age > config::portal_exit_max_age) {
            continue;
        }
        auto recent_enemy = false;
        for (auto y = 0; y < world.height() && !recent_enemy; ++y) {
            for (auto x = 0; x < world.width(); ++x) {
                auto const p = unswbc::Position{x, y};
                auto const& cell = world.cell(p);
                if (cell.occupant && cell.occupant->is_head
                    && cell.occupant->team != controller.get_team().value
                    && round - cell.last_seen_round <= 2
                    && geometry::toroidal_manhattan(p, *exit, world.width(), world.height()) <= 4) {
                    recent_enemy = true;
                    break;
                }
            }
        }
        if (recent_enemy) {
            continue;
        }
        auto onward = std::vector<unswbc::Position>{};
        for (auto const d : unswbc::Direction::get_direction_list()) {
            auto const next = world.transition(*exit, d);
            if (next && world.has_seen(*next) && !blocked(*next)
                && std::find(onward.begin(), onward.end(), *next) == onward.end()) {
                onward.push_back(*next);
            }
        }
        if (onward.size() < 2) {
            continue;
        }
        auto queue = std::vector<unswbc::Position>{*exit};
        for (std::size_t cursor = 0; cursor < queue.size() && cursor < 64U; ++cursor) {
            for (auto const d : unswbc::Direction::get_direction_list()) {
                auto const next = world.transition(queue[cursor], d);
                if (next && world.has_seen(*next) && !blocked(*next)
                    && std::find(queue.begin(), queue.end(), *next) == queue.end()) {
                    queue.push_back(*next);
                }
            }
        }
        if (queue.size() < state.body.size() + 3) {
            continue;
        }
        auto const score = static_cast<int>(onward.size()) * 1000 + static_cast<int>(std::min<std::size_t>(queue.size(), 64U)) * 10 - age * 20;
        if (score > best_score) {
            best = direction;
            best_score = score;
        }
    }
    return best;
}

auto Safety::least_bad_fallback(unswbc::Controller const& controller) const -> unswbc::Direction {
    auto best = controller.get_dir();
    auto best_rank = 7;
    // Prefer an uncertain escape over a certainly fatal wall/body collision.
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        auto const reason = standard_move_reason(controller, direction);
        auto rank = reason == SafetyReason::safe ? 0
                        : reason == SafetyReason::unknown_tile ? 1
                        : reason == SafetyReason::unknown_portal ? 2
                        : reason == SafetyReason::occupied ? 4 : 5;
        if (reason == SafetyReason::occupied) {
            auto const* tile = controller.get_tile(controller.get_position().add_dir(direction));
            auto const* part = tile != nullptr ? tile->get_dragon() : nullptr;
            if (part != nullptr && part->is_head() && part->get_id() != controller.get_id()) {
                // When every escape fails, never kill an ally as well. An
                // enemy head can at least turn the forced loss into a trade.
                rank = part->get_team() == controller.get_team() ? 6 : 3;
            }
        }
        if (rank < best_rank || (rank == best_rank && direction == controller.get_dir())) {
            best = direction;
            best_rank = rank;
        }
    }
    return best;
}

} // namespace sudo_win

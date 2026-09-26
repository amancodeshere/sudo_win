#include "../../include/sudo_win/planner/planner.h"

#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/world/world_model.h"

namespace sudo_win {

auto Planner::choose_action(unswbc::Controller const& controller,
                            unswbc::Game const& game,
                            WorldModel const& world,
                            Role role) const -> PlannedAction {
    auto const safe_moves = safety_.safe_standard_moves(controller);
    auto best = PlannedAction{};
    auto largest_reachable_area = 0;

    for (auto const direction : safe_moves) {
        auto const destination = controller.get_position().add_dir(direction);
        auto const reachable_area = pathfinding_.visible_reachable_area(controller, destination);
        largest_reachable_area = reachable_area > largest_reachable_area ? reachable_area : largest_reachable_area;

        auto candidate = MoveCandidate{direction};
        candidate.mobility_score = reachable_area * config::score_reachable_tile;
        candidate.economy_score = economy_.score_destination(controller, world, pathfinding_, destination);
        candidate.exploration_score = world.unseen_neighbour_count(destination) * config::score_frontier;
        candidate.combat_score = combat_.destination_risk(controller, destination, role);
        candidate.role_score = roles_.score_move(role,
                                                 reachable_area,
                                                 world.unseen_neighbour_count(destination),
                                                 candidate.combat_score);
        candidate.endgame_score = endgame_.score_destination(controller,
                                                             game,
                                                             role,
                                                             reachable_area,
                                                             candidate.combat_score);

        if (direction == controller.get_dir()) {
            candidate.mobility_score += config::score_facing_continuity;
        }
        if (direction == controller.get_dir().get_opposite()) {
            candidate.mobility_score += config::score_reverse;
        }

        if (candidate.total_score() > best.score) {
            best.kind = ActionKind::move;
            best.steps = {direction};
            best.score = candidate.total_score();
            best.reason = "best safe move";
        }
    }

    if (auto const split = splitting_.consider(controller, game, role, largest_reachable_area);
        split.has_value() && split->score > best.score) {
        return *split;
    }

    if (best.steps.empty()) {
        best.kind = ActionKind::move;
        best.steps = {safety_.least_bad_fallback(controller)};
        best.reason = "mandatory fallback";
    }
    return best;
}

} // namespace sudo_win

#include "../../include/sudo_win/planner/planner.h"

#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/planner/simulation.h"
#include "../../include/sudo_win/world/world_model.h"

namespace sudo_win {

auto Planner::choose_action(unswbc::Controller const& controller,
                            unswbc::Game const& game,
                            WorldModel const& world,
                            Role role) const -> PlannedAction {
    auto const safe_moves = safety_.safe_standard_moves(controller);
    auto best = PlannedAction{};
    auto largest_reachable_area = 0;
    auto const simulation = Simulation{};
    auto const initial = simulation.initial_state(controller);
    auto best_survival = -1;
    if (game.get_round_num() - target_round_ > config::target_max_age) {
        target_.reset();
    }
    auto const route = pathfinding_.remembered_target(controller, world, game.get_round_num(), target_);
    if (route) {
        if (!target_ || *target_ != route->target) {
            target_round_ = game.get_round_num();
        }
        target_ = route->target;
    } else {
        target_.reset();
    }

    for (auto const direction : safe_moves) {
        auto const next = simulation.advance(controller, initial, direction);
        if (!next) {
            continue;
        }
        auto budget = config::survival_node_budget;
        auto const survival = simulation.survival_depth(controller, *next,
                                                         config::survival_search_depth, budget);
        auto const destination = controller.get_position().add_dir(direction);
        auto const reachable_area = pathfinding_.visible_reachable_area(controller, destination);
        largest_reachable_area = reachable_area > largest_reachable_area ? reachable_area : largest_reachable_area;

        auto candidate = MoveCandidate{direction};
        candidate.mobility_score = reachable_area * config::score_reachable_tile;
        candidate.economy_score = economy_.score_destination(controller, world, pathfinding_, destination);
        if (route && route->first_direction == direction) {
            candidate.economy_score += config::score_route_progress + route->value;
        }
        auto const last_visit = world.cell(destination).last_visited_round;
        if (last_visit >= 0 && game.get_round_num() - last_visit < 4
            && !world.cell(destination).has_pearl) {
            candidate.exploration_score += config::score_recent_visit;
        }
        candidate.exploration_score += world.unseen_neighbour_count(destination) * config::score_frontier;
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

        if (survival > best_survival || (survival == best_survival && candidate.total_score() > best.score)) {
            best_survival = survival;
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

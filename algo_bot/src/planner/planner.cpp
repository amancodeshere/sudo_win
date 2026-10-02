#include "../../include/sudo_win/planner/planner.h"

#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/geometry/geometry.h"
#include "../../include/sudo_win/planner/simulation.h"
#include "../../include/sudo_win/world/world_model.h"

#include <algorithm>

namespace sudo_win {

auto Planner::choose_action(unswbc::Controller const& controller,
                            unswbc::Game const& game,
                            WorldModel const& world,
                            Role role) const -> PlannedAction {
    if (resource_progress_round_ < 0 || controller.get_length() > previous_length_) {
        resource_progress_round_ = game.get_round_num();
    }
    previous_length_ = controller.get_length();
    if ((favourable_trades_ || config::enable_queen_hunting) && role != Role::queen) {
        if (auto const trade = combat_.favourable_trade(controller, world, !favourable_trades_)) {
            return *trade;
        }
    }
    auto const safe_moves = safety_.safe_standard_moves(controller, &world);
    auto best = PlannedAction{};
    auto largest_reachable_area = 0;
    auto const simulation = Simulation{};
    auto const threats = combat_.threats(controller, &world);
    auto const initial = simulation.initial_state(controller, &world);
    auto const reservations = world.queen_reservations(controller, game.get_round_num());
    auto const interception = combat_.interception_distances(controller, world, game.get_round_num(), role);
    auto const interception_start = interception[static_cast<std::size_t>(controller.get_position().y * world.width() + controller.get_position().x)];
    auto best_survival = -1;
    auto best_safety_class = -1;
    auto best_sealed_entry = false;
    if (game.get_round_num() - target_round_ > config::target_max_age) {
        target_.reset();
    }
    auto const route = pathfinding_.remembered_target(controller, world, game.get_round_num(), target_,
        role == Role::queen || role == Role::champion,
        role == Role::scout && controller.get_length() <= 4 && controller.get_unit_count() > 1);
    if (route) {
        if (!target_ || *target_ != route->target) {
            target_round_ = game.get_round_num();
        }
        target_ = route->target;
    } else {
        target_.reset();
    }

    auto const consider = [&](SimulationState const& next, std::vector<unswbc::Direction> const& steps) {
        auto const direction = steps.front();
        auto const sprint = steps.size() > 1;
        auto budget = config::survival_node_budget;
        auto const survival = simulation.survival_depth(controller, next,
                                                         config::survival_search_depth, budget, &world);
        auto const destination = next.body.front();
        auto const reachable_area = simulation.reachable_area(controller, next, &world);
        largest_reachable_area = reachable_area > largest_reachable_area ? reachable_area : largest_reachable_area;

        auto candidate = MoveCandidate{direction};
        candidate.mobility_score = reachable_area * config::score_reachable_tile;
        auto const mobility = simulation.remembered_mobility(controller, next, world);
        candidate.mobility_score += mobility.area * config::score_remembered_tile;
        if (!mobility.open_frontier && mobility.area < static_cast<int>(next.body.size()) + 3) {
            candidate.mobility_score += config::score_closed_pocket;
        }
        candidate.economy_score = economy_.score_destination(controller, world, pathfinding_, destination);
        auto const length_gain = static_cast<int>(next.body.size()) - controller.get_length();
        auto const* tile = controller.get_tile(destination);
        // Economy scores the endpoint pearl; replace that with the full route's
        // net growth, including every extra-step payment and intermediate pearl.
        candidate.economy_score += length_gain * config::score_immediate_pearl;
        if (tile != nullptr && tile->has_pearl()) {
            candidate.economy_score -= config::score_immediate_pearl;
        }
        if (sprint) {
            auto const first_target = world.transition(controller.get_position(), direction);
            auto const* first_tile = first_target ? controller.get_tile(*first_target) : nullptr;
            auto const first_pearl = first_tile != nullptr && first_tile->has_pearl() ? 1 : 0;
            candidate.economy_score += (next.pearls - first_pearl) * config::score_sprint_tempo;
        }
        if (route && route->first_direction == direction) {
            candidate.economy_score += config::score_route_progress + route->value;
        }
        auto const last_visit = world.cell(destination).last_visited_round;
        if (last_visit >= 0 && game.get_round_num() - last_visit < 4
            && !world.cell(destination).has_pearl) {
            candidate.exploration_score += config::score_recent_visit;
        }
        candidate.exploration_score += world.unseen_neighbour_count(destination) * config::score_frontier;
        // Every retained segment can obstruct the queen, not only our head.
        for (auto const p : next.body) {
            if (p.x >= 0 && p.y >= 0) {
                candidate.role_score -= reservations[static_cast<std::size_t>(p.y * world.width() + p.x)];
            }
        }
        auto const interception_end = interception[static_cast<std::size_t>(destination.y * world.width() + destination.x)];
        if (interception_start > 0 && interception_end >= 0 && interception_end < interception_start) {
            candidate.role_score += (interception_start - interception_end) * 18000 + (8 - interception_end) * 1000;
        }
        for (auto const& report : world.reports()) {
            if (report.type != MessageType::enemy_head || report.value != 1024
                || game.get_round_num() - report.round > 2) {
                continue;
            }
            auto const enemy = unswbc::Position{report.x, report.y};
            auto const distance = geometry::toroidal_manhattan(destination, enemy, world.width(), world.height());
            if (role == Role::queen && distance <= 3) {
                candidate.exploration_score -= (4 - distance) * 4000;
            } else if (controller.get_id() > 1 && controller.get_length() <= 3
                && controller.get_unit_count() > 1 && config::enable_queen_hunting) {
                candidate.exploration_score += std::max(0, 8 - distance) * 1000;
            }
        }
        auto const& threat = threats[static_cast<std::size_t>(destination.y * world.width() + destination.x)];
        candidate.combat_score = threat.score * (role == Role::queen ? 3
            : role == Role::champion ? config::score_champion_risk_multiplier : 1);
        auto const threatened = threat.level == ThreatLevel::direct
            || ((role == Role::queen || controller.get_unit_count() == 1)
                && threat.later_affordable_steps > 0 && threat.later_affordable_steps <= 3)
            || (funded_sprint_priority_ && threat.affordable_steps > 0 && threat.affordable_steps <= 2);
        auto const entry_trap = steps.size() == 1 && survival < config::survival_search_depth
            && simulation.sealed_entry_pocket(next, world);
        auto const sealed_pocket = config::enable_pocket_priority && !mobility.open_frontier
            && mobility.area < static_cast<int>(next.body.size())
            && simulation.sealed_entry_pocket(next, world);
        auto const safety_class = survival == 0 ? 0 : (threatened || sealed_pocket) ? 1 : 2;
        auto const improves_safety = safety_class > best_safety_class
            || (safety_class == best_safety_class && survival > best_survival && best_sealed_entry);
        if (sprint && length_gain < 0 && !improves_safety) {
            return;
        }
        auto const paid_steps = static_cast<int>(steps.size()) - Simulation::free_steps(controller.get_length());
        if (sprint && paid_steps > 0 && length_gain <= 0
            && (config::enable_movement_economics || role == Role::champion || role == Role::queen || endgame_.active(game))
            && !improves_safety) {
            return;
        }
        candidate.role_score += roles_.score_move(role,
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

        if (safety_class > best_safety_class
            || (safety_class == best_safety_class && (survival > best_survival
                || (survival == best_survival && candidate.total_score() > best.score)))) {
            best_safety_class = safety_class;
            best_survival = survival;
            best_sealed_entry = entry_trap;
            best.kind = sprint ? ActionKind::sprint : ActionKind::move;
            best.steps = steps;
            best.score = candidate.total_score();
            best.reason = sprint ? "validated short sprint" : "best safe move";
        }
    };

    for (auto const direction : safe_moves) {
        if (auto const next = simulation.advance(controller, initial, direction, false, &world)) {
            consider(*next, {direction});
        }
    }
    if (sprinting_) {
        struct SprintNode { SimulationState state; std::vector<unswbc::Direction> steps; int rank; };
        auto beam = std::vector<SprintNode>{};
        for (auto const direction : safe_moves) {
            if (auto const next = simulation.advance(controller, initial, direction, false, &world)) {
                beam.push_back({*next, {direction}, 0});
            }
        }
        auto const step_limit = std::min(config::free_sprint_step_cap,
            std::max(static_cast<int>(config::max_sprint_steps), Simulation::free_steps(controller.get_length())));
        auto remaining_nodes = config::free_sprint_node_budget;
        for (auto depth = 2; depth <= step_limit && !beam.empty() && remaining_nodes > 0; ++depth) {
            auto expanded = std::vector<SprintNode>{};
            for (auto const& node : beam) {
                for (auto const direction : unswbc::Direction::get_direction_list()) {
                    if (remaining_nodes-- <= 0) {
                        break;
                    }
                    auto next = simulation.advance(controller, node.state, direction, true, &world);
                    if (!next) {
                        continue;
                    }
                    auto steps = node.steps;
                    steps.push_back(direction);
                    auto const p = next->body.front();
                    auto const gain = static_cast<int>(next->body.size()) - controller.get_length();
                    auto const rank = gain * config::score_immediate_pearl
                        + simulation.reachable_area(controller, *next, &world) * config::score_reachable_tile
                        + threats[static_cast<std::size_t>(p.y * world.width() + p.x)].score;
                    expanded.push_back({std::move(*next), std::move(steps), rank});
                }
            }
            std::stable_sort(expanded.begin(), expanded.end(), [](auto const& a, auto const& b) {
                return a.rank > b.rank;
            });
            beam.clear();
            auto retained = std::array<int, 4>{};
            for (auto& node : expanded) {
                auto const first = geometry::direction_index(node.steps.front());
                if (retained[first]++ >= config::sprint_beam_per_direction) {
                    continue;
                }
                consider(node.state, node.steps);
                beam.push_back(std::move(node));
            }
        }
    }

    if (config::enable_portal_routing && (role == Role::queen || role == Role::champion)
        && game.get_round_num() < 400 && game.get_round_num() - last_portal_round_ >= 12
        && game.get_round_num() - resource_progress_round_ >= 12 && (!route || !route->pearl)) {
        if (auto const portal = safety_.surveyed_portal_route(controller, world, game.get_round_num())) {
            last_portal_round_ = game.get_round_num();
            best.kind = ActionKind::move;
            best.steps = {*portal};
            best.reason = "starved protected unit follows a fresh advisory portal survey";
            return best;
        }
    }
    auto const scout_wait = config::enable_portal_routing && role == Role::scout ? 3 : 8;
    if (config::enable_helper_portals && role != Role::queen && role != Role::champion
        && game.get_round_num() - last_portal_round_ >= 12
        && (best_survival <= 1 || ((!route || !route->pearl)
            && game.get_round_num() - resource_progress_round_ >= scout_wait
            && (!config::enable_portal_routing || role != Role::scout
                || pathfinding_.visible_pearl_distance(controller, controller.get_position(), &world) > 3)))) {
        if (auto const portal = safety_.helper_portal_probe(controller, world, game.get_round_num())) {
            last_portal_round_ = game.get_round_num();
            best.kind = ActionKind::move;
            best.steps = {*portal};
            best.reason = "small helper samples portal after local resource exhaustion";
            return best;
        }
    }

    auto rescue = std::optional<PlannedAction>{};
    if (best_survival <= 1) {
        rescue = splitting_.rescue(controller, world, best.steps.empty(), role == Role::queen);
        if (rescue && rescue->score > 0) {
            return *rescue;
        }
    }

    if (config::enable_portal_escape && (best.steps.empty()
        || (role != Role::queen && best_survival <= 2))) {
        if (auto const portal = safety_.remembered_portal_escape(controller, world, game.get_round_num())) {
            best.kind = ActionKind::move;
            best.steps = {*portal};
            best.reason = "uncertain portal escape through remembered empty exit";
            return best;
        }
    }

    if (rescue) {
        return *rescue;
    }

    if (growth_splitting_) {
        if (auto const expansion = splitting_.grow_population(controller, game, role, world);
            expansion && expansion->score > best.score) {
            return *expansion;
        }
    }

    if (auto const split = splitting_.consider(controller, game, role, largest_reachable_area, &world, splitting_enabled_);
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

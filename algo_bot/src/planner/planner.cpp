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
    growth_rejection_ = "movement or rescue priority";
    auto const verified_collection = config::enable_paid_step_pricing && collection_head_
        && collection_round_ + 1 == game.get_round_num() && *collection_head_ == controller.get_position()
        && collection_length_ == controller.get_length();
    if (resource_progress_round_ < 0 || controller.get_length() > previous_length_ || verified_collection) {
        resource_progress_round_ = game.get_round_num();
    }
    previous_length_ = controller.get_length();
    auto const finish = [&](PlannedAction action) {
        collection_head_.reset();
        if (config::enable_paid_step_pricing && action.kind != ActionKind::split && !action.steps.empty()) {
            auto const simulation = Simulation{};
            auto state = simulation.initial_state(controller, &world);
            auto complete = true;
            for (std::size_t i = 0; i < action.steps.size(); ++i) {
                auto const next = simulation.advance(controller, state, action.steps[i], i > 0, &world);
                if (!next) { complete = false; break; }
                state = *next;
            }
            if (complete && state.pearls > 0) {
                collection_head_ = state.body.front(); collection_length_ = static_cast<int>(state.body.size());
                collection_round_ = game.get_round_num();
            }
        }
        return action;
    };
    if ((favourable_trades_ || config::enable_queen_hunting) && role != Role::queen) {
        if (auto const trade = combat_.favourable_trade(controller, world, !favourable_trades_)) {
            return finish(*trade);
        }
    }
    auto const safe_moves = safety_.safe_standard_moves(controller, &world);
    auto best = PlannedAction{};
    auto largest_reachable_area = 0;
    auto const simulation = Simulation{};
    auto const threats = combat_.threats(controller, &world);
    auto const initial = simulation.initial_state(controller, &world);
    auto const reservations = world.queen_reservations(controller, game.get_round_num());
    // Unranked visible parts still block the queen. Extra free movement sheds
    // tail segments without pretending to know their order or clearing occupancy.
    auto const blocks_queen = config::enable_tail_clearance && controller.get_id() > 1
        && std::any_of(controller.get_tiles().begin(), controller.get_tiles().end(), [&](auto const& tile) {
            auto const* part = tile.get_dragon();
            auto const p = tile.get_position();
            return part != nullptr && part->get_id() == controller.get_id()
                && reservations[static_cast<std::size_t>(p.y * world.width() + p.x)] >= 16000;
        });
    auto const interception = combat_.interception_distances(controller, world, game.get_round_num(), role);
    auto const interception_start = interception[static_cast<std::size_t>(controller.get_position().y * world.width() + controller.get_position().x)];
    auto best_survival = -1;
    auto best_safety_class = -1;
    auto best_sealed_entry = false;
    if (game.get_round_num() - target_round_ > config::target_max_age) {
        target_.reset();
    }
    auto route = pathfinding_.remembered_target(controller, world, game.get_round_num(), target_,
        role == Role::queen || role == Role::champion,
        role == Role::scout && controller.get_length() <= 4 && controller.get_unit_count() > 1);
    if (config::enable_portal_income && game.get_round_num() < 400
        && game.get_round_num() - last_portal_round_ >= 12
        && game.get_round_num() - resource_progress_round_ >= 8
        && (!route || !route->pearl || (role != Role::queen && role != Role::champion && route->value < 8000))) {
        auto const relocation = pathfinding_.portal_income_route(controller, world, game.get_round_num(),
            role == Role::queen || (role == Role::champion && controller.get_length() > 4));
        if (relocation && (!route || relocation->value > route->value)) { route = relocation; }
    }
    if (route) {
        if (!target_ || *target_ != route->target) {
            target_round_ = game.get_round_num();
        }
        target_ = route->target;
    } else {
        target_.reset();
    }

    auto const route_distances = config::enable_scoring_coordination && route
        ? pathfinding_.target_distances(controller, world, route->target, route->portal) : std::vector<int>{};
    auto const route_index = [&world](unswbc::Position p) {
        return static_cast<std::size_t>(p.y * world.width() + p.x);
    };

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
        auto route_progress = false;
        if (route) {
            route_progress = route->first_direction == direction;
            if (config::enable_scoring_coordination) {
                auto const origin_distance = route_distances[route_index(controller.get_position())];
                auto const endpoint_distance = route_distances[route_index(destination)];
                auto const collected = std::find(next.eaten.begin(), next.eaten.end(), route->target) != next.eaten.end();
                auto const crossing = route->portal && route->target == controller.get_position()
                    && route->first_direction == direction;
                route_progress = collected || crossing || (origin_distance > 0
                    && endpoint_distance >= 0 && endpoint_distance < origin_distance);
            }
            if (route_progress) { candidate.economy_score += config::score_route_progress + route->value; }
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
        if (blocks_queen) {
            candidate.role_score += std::max(0, std::min(static_cast<int>(steps.size()),
                Simulation::free_steps(controller.get_length())) - 1) * 6000;
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
        auto const protected_unit = role == Role::queen || role == Role::champion || controller.get_unit_count() == 1;
        auto const response = config::enable_response_defense && protected_unit
            ? combat_.response_threat(controller, next, world) : ResponseThreat{};
        auto const threatened = threat.level == ThreatLevel::direct
            || ((role == Role::queen || controller.get_unit_count() == 1)
                && threat.later_affordable_steps > 0 && threat.later_affordable_steps <= 3)
            || (funded_sprint_priority_ && threat.affordable_steps > 0 && threat.affordable_steps <= 2)
            || response.funded_steps > 0;
        auto const uncertain_attack = response.possible_steps > 0 && response.possible_steps <= 3;
        if (response.possible_steps > 0) {
            candidate.combat_score -= (6 - response.possible_steps) * 10000;
        }
        auto const entry_trap = steps.size() == 1 && survival < config::survival_search_depth
            && simulation.sealed_entry_pocket(next, world);
        auto const sealed_pocket = config::enable_pocket_priority && !mobility.open_frontier
            && mobility.area < static_cast<int>(next.body.size())
            && simulation.sealed_entry_pocket(next, world);
        auto const safety_class = survival == 0 ? 0 : (threatened || sealed_pocket) ? 1 : uncertain_attack ? 2 : 3;
        auto const improves_safety = safety_class > best_safety_class
            || (safety_class == best_safety_class && survival > best_survival && best_sealed_entry);
        auto const releases_queen = config::enable_queen_release && controller.get_id() > 1
            && controller.get_length() <= 4 && controller.get_unit_count() > 1 && safety_class == 3
            && survival == config::survival_search_depth
            && std::any_of(initial.body.begin(), initial.body.end(), [&](auto const p) {
                return p.x >= 0 && p.y >= 0
                    && reservations[static_cast<std::size_t>(p.y * world.width() + p.x)] >= 120000
                    && std::find(next.body.begin(), next.body.end(), p) == next.body.end()
                    && std::find(next.unranked_body.begin(), next.unranked_body.end(), p) == next.unranked_body.end();
            });
        if (sprint && length_gain < 0 && !improves_safety && !releases_queen) {
            return;
        }
        auto const paid_steps = static_cast<int>(steps.size()) - Simulation::free_steps(controller.get_length());
        if (sprint && paid_steps > 0 && length_gain <= 0
            && (config::enable_movement_economics || role == Role::champion || role == Role::queen || endgame_.active(game))
            && !improves_safety && !releases_queen) {
            return;
        }
        if (config::enable_paid_step_pricing && paid_steps > 0 && !improves_safety && !releases_queen) {
            candidate.economy_score -= paid_steps * config::score_paid_step_cost;
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
            best.reason = config::enable_paid_step_pricing && paid_steps > 0
                ? (releases_queen ? "paid queen corridor release" : length_gain > 0 ? "paid income investment" : "paid validated escape")
                : route && route->portal && route_progress ? "approach productive or unexplored portal"
                : sprint ? "validated free sprint" : "best safe move";
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
                        - (config::enable_paid_step_pricing ? std::max(0,depth - Simulation::free_steps(controller.get_length()))
                            * config::score_paid_step_cost : 0)
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
            return finish(best);
        }
    }
    auto const scout_wait = config::enable_portal_routing && role == Role::scout ? 3 : 8;
    if (config::enable_helper_portals && role != Role::queen && (role != Role::champion || (config::enable_scoring_coordination && controller.get_length() <= 4))
        && game.get_round_num() - last_portal_round_ >= 12
        && (best_survival <= 1 || ((!route || !route->pearl || (config::enable_portal_income && route->value < 8000))
            && game.get_round_num() - resource_progress_round_ >= scout_wait
            && (!config::enable_portal_routing || role != Role::scout
                || pathfinding_.visible_pearl_distance(controller, controller.get_position(), &world) > 3)))) {
        if (auto const portal = safety_.helper_portal_probe(controller, world, game.get_round_num())) {
            last_portal_round_ = game.get_round_num();
            best.kind = ActionKind::move;
            best.steps = {*portal};
            best.reason = "small helper samples portal after local resource exhaustion";
            return finish(best);
        }
    }

    auto rescue = std::optional<PlannedAction>{};
    if (best_survival <= 1) {
        rescue = splitting_.rescue(controller, world, best.steps.empty(), role == Role::queen);
        if (rescue && rescue->score > 0) {
            return finish(*rescue);
        }
    }

    if (config::enable_portal_escape && (best.steps.empty()
        || (role != Role::queen && best_survival <= 2))) {
        if (auto const portal = safety_.remembered_portal_escape(controller, world, game.get_round_num())) {
            best.kind = ActionKind::move;
            best.steps = {*portal};
            best.reason = "uncertain portal escape through remembered empty exit";
            return finish(best);
        }
    }

    if (rescue) {
        return finish(*rescue);
    }

    if (growth_splitting_) {
        auto const expansion = splitting_.grow_population(controller, game, role, world);
        growth_rejection_ = splitting_.growth_rejection();
        if (expansion) {
            if (expansion->score > best.score) {
                if (expansion->resource_target) { target_ = expansion->resource_target; target_round_ = game.get_round_num(); }
                return finish(*expansion);
            }
            growth_rejection_ = "move exceeds investment";
        }
    }

    if (auto const split = splitting_.consider(controller, game, role, largest_reachable_area, &world, splitting_enabled_);
        split.has_value() && split->score > best.score) {
        return finish(*split);
    }

    if (best.steps.empty()) {
        best.kind = ActionKind::move;
        best.steps = {safety_.least_bad_fallback(controller)};
        best.reason = "mandatory fallback";
    }
    return finish(best);
}

} // namespace sudo_win

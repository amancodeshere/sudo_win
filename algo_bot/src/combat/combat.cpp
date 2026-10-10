#include "../../include/sudo_win/combat/combat.h"

#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/geometry/geometry.h"
#include "../../include/sudo_win/planner/simulation.h"
#include "../../include/sudo_win/world/world_model.h"
#include <algorithm>

namespace sudo_win {

auto Combat::response_threat(unswbc::Controller const& controller,
                             SimulationState const& after, WorldModel const& world,
                             int node_budget) const -> ResponseThreat {
    auto result = ResponseThreat{};
    if (after.body.empty()) { return result; }
    auto view = controller;
    // The enemy responds to our resulting occupancy, not the departing tail.
    // Leave the target head empty only for the terminal head-to-head test.
    for (auto& tile : view.vision.tiles) {
        if (tile.dragon_part && tile.dragon_part->get_id() == controller.get_id()) {
            tile.dragon_part.reset();
        }
        if (std::find(after.eaten.begin(), after.eaten.end(), tile.get_position()) != after.eaten.end()) {
            tile.pearl = false;
        }
    }
    auto const block = [&](unswbc::Position p) {
        if (auto* tile = view.get_tile(p)) {
            tile->dragon_part = unswbc::DragonPart{p, controller.get_id(), controller.get_team(),
                                                  controller.get_dir(), false};
        }
    };
    for (std::size_t i = 1; i < after.body.size(); ++i) { block(after.body[i]); }
    for (auto const p : after.unranked_body) { block(p); }
    auto const target = after.body.front();
    auto enemies = std::vector<unswbc::DragonPart>{};
    for (auto const& tile : view.get_tiles()) {
        auto const* part = tile.get_dragon();
        // Every visible enemy acts before our next turn: higher IDs this
        // round, lower IDs next round. Neither gets two simulated actions.
        if (part != nullptr && part->is_head() && part->get_team() != controller.get_team()) {
            enemies.push_back(*part);
        }
    }
    if (enemies.empty()) { return result; }
    // A reverse lower bound on the directed visible topology prunes detours.
    // Ignore occupancy here: moving enemy tails can open cells during a sprint.
    auto const index = [&](unswbc::Position p) {
        return static_cast<std::size_t>(p.y * world.width() + p.x);
    };
    auto reverse = std::vector<std::vector<unswbc::Position>>(
        static_cast<std::size_t>(world.width() * world.height()));
    for (auto const& tile : view.get_tiles()) {
        for (auto const d : unswbc::Direction::get_direction_list()) {
            auto const next = world.transition(tile.get_position(), d);
            if (next && view.get_tile(*next) != nullptr) {
                reverse[index(*next)].push_back(tile.get_position());
            }
        }
    }
    auto distance = std::vector<int>(reverse.size(), -1);
    auto frontier = std::vector<unswbc::Position>{target};
    distance[index(target)] = 0;
    for (std::size_t cursor = 0; cursor < frontier.size(); ++cursor) {
        auto const p = frontier[cursor];
        if (distance[index(p)] >= config::response_step_limit) { continue; }
        for (auto const previous : reverse[index(p)]) {
            if (distance[index(previous)] < 0) {
                distance[index(previous)] = distance[index(p)] + 1;
                frontier.push_back(previous);
            }
        }
    }
    std::stable_sort(enemies.begin(),enemies.end(),[&](auto const& a, auto const& b) {
        auto const rank = [&](auto const& enemy) {
            auto const d = distance[index(enemy.get_position())];
            return d < 0 ? config::response_step_limit + 1 : d;
        };
        return rank(a) < rank(b);
    });
    auto budget = node_budget;
    auto const simulation = Simulation{};
    auto const unresolved = [&](int steps) {
        if (result.unresolved_steps == 0 || steps < result.unresolved_steps) {
            result.unresolved_steps = steps;
        }
    };
    for (auto const& enemy : enemies) {
        if (distance[index(enemy.get_position())] < 0) { continue; }
        auto observed = 0;
        for (auto const& tile : view.get_tiles()) {
            observed += tile.dragon_part && tile.dragon_part->get_id() == enemy.get_id();
        }
        auto opponent = view;
        opponent.head = enemy;
        opponent.length = std::max(5, observed + 2);
        auto const known_start = std::max(unswbc::Constants::MIN_SIZE, observed);
        struct Node { SimulationState state; int steps; bool funded; int known_length; };
        auto queue = std::vector<Node>{{simulation.initial_state(opponent, &world), 0, true, known_start}};
        auto enemy_budget = 64;
        for (std::size_t cursor = 0; cursor < queue.size(); ++cursor) {
            auto const node = queue[cursor];
            if (node.steps >= config::response_step_limit) { continue; }
            // Shortest plausible attacks first; directed portal distances, not
            // Manhattan distance, determine which branches can reach the head.
            auto directions = unswbc::Direction::get_direction_list();
            std::stable_sort(directions.begin(), directions.end(), [&](auto a, auto b) {
                auto const rank = [&](auto d) {
                    auto const p = world.transition(node.state.body.front(), d);
                    return p && view.get_tile(*p) != nullptr && distance[index(*p)] >= 0
                        ? distance[index(*p)] : config::response_step_limit + 1;
                };
                return rank(a) < rank(b);
            });
            for (auto const d : directions) {
                auto const p = world.transition(node.state.body.front(), d);
                if (!p || view.get_tile(*p) == nullptr || distance[index(*p)] < 0
                    || node.steps + 1 + distance[index(*p)] > config::response_step_limit) { continue; }
                // Every enemy's first step is checked even if another enemy
                // used the continuation budget. Preserve unfinished routes.
                if (node.steps > 0) {
                    if (budget <= 0 || enemy_budget <= 0) {
                        unresolved(node.steps + 1 + distance[index(*p)]);
                        continue;
                    }
                    --budget;
                    --enemy_budget;
                }
                auto next = simulation.advance(opponent, node.state, d, node.steps > 0, &world);
                if (!next) { continue; }
                auto const steps = node.steps + 1;
                // The uncertain length envelope has a different free-step
                // allowance. Track proven funding independently so its extra
                // free movement cannot certify a minimum-length enemy sprint.
                auto const paid = node.steps >= Simulation::free_steps(known_start);
                auto const funded = node.funded && (!paid || node.known_length > unswbc::Constants::MIN_SIZE);
                auto const known_length = node.known_length + next->pearls - node.state.pearls - (paid ? 1 : 0);
                if (next->body.front() == target) {
                    if (result.possible_steps == 0 || steps < result.possible_steps) { result.possible_steps = steps; }
                    if (funded && (result.funded_steps == 0 || steps < result.funded_steps)) { result.funded_steps = steps; }
                    continue;
                }
                queue.push_back({std::move(*next), steps, funded, known_length});
            }
        }
    }
    return result;
}

auto Combat::interception_distances(unswbc::Controller const& controller,
                                     WorldModel const& world, int round, Role role) const -> std::vector<int> {
    auto distances = std::vector<int>(static_cast<std::size_t>(world.width() * world.height()), -1);
    if (!config::enable_interception || controller.get_id() <= 1 || controller.get_length() > 4
        || controller.get_unit_count() <= 1 || (role != Role::hunter && role != Role::blocker)) {
        return distances;
    }
    auto queen = std::optional<unswbc::Position>{};
    auto nearest = std::numeric_limits<int>::max();
    auto const accept = [&](unswbc::Position p) {
        auto const distance = geometry::toroidal_manhattan(controller.get_position(), p, world.width(), world.height());
        if (distance < nearest) {
            nearest = distance;
            queen = p;
        }
    };
    for (auto const& tile : controller.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (part != nullptr && part->is_head() && part->get_id() <= 1 && part->get_team() != controller.get_team()) {
            accept(tile.get_position());
        }
    }
    if (!queen) {
        for (auto const& report : world.reports()) {
            if (report.type == MessageType::enemy_head && report.value == 1024
                && round >= report.round && round - report.round <= 2) {
                auto const p = unswbc::Position{report.x, report.y};
                // A present observation takes precedence over an advisory sighting.
                if (controller.get_tile(p) == nullptr) {
                    accept(p);
                }
            }
        }
    }
    if (!queen || nearest > 9) {
        return distances;
    }
    auto const available = [&](unswbc::Position p) {
        if (!world.has_seen(p)) {
            return false;
        }
        auto const* tile = controller.get_tile(p);
        auto const* part = tile != nullptr ? tile->get_dragon() : nullptr;
        return part == nullptr || (part->get_id() == controller.get_id() && part->is_head());
    };
    auto goal = std::optional<unswbc::Position>{};
    auto const directions = unswbc::Direction::get_direction_list();
    auto const offset = (controller.get_id() / 4 + (role == Role::blocker ? 1 : 0)) % 4;
    for (auto i = 0; i < 4; ++i) {
        auto const target = world.transition(*queen, directions[static_cast<std::size_t>((offset + i) % 4)]);
        if (target && available(*target)) {
            goal = target;
            break;
        }
    }
    if (!goal) {
        return distances;
    }
    auto const index = [&](unswbc::Position p) { return static_cast<std::size_t>(p.y * world.width() + p.x); };
    distances[index(*goal)] = 0;
    auto queue = std::vector<unswbc::Position>{*goal};
    for (std::size_t cursor = 0; cursor < queue.size() && cursor < 128; ++cursor) {
        auto const p = queue[cursor];
        if (distances[index(p)] >= 8) {
            continue;
        }
        for (auto const direction : directions) {
            auto const neighbour = world.transition(p, direction);
            if (!neighbour || !available(*neighbour) || distances[index(*neighbour)] >= 0) {
                continue;
            }
            auto const reverse = world.transition(*neighbour, direction.get_opposite());
            if (!reverse || *reverse != p) {
                continue;
            }
            distances[index(*neighbour)] = distances[index(p)] + 1;
            queue.push_back(*neighbour);
        }
    }
    return distances;
}

auto Combat::favourable_trade(unswbc::Controller const& controller,
                              WorldModel const& world, bool queen_only) const -> std::optional<PlannedAction> {
    if (controller.get_id() <= 1 || controller.get_length() > 3 || controller.get_unit_count() <= 1) {
        return std::nullopt;
    }
    auto const simulation = Simulation{};
    struct Node { SimulationState state; std::vector<unswbc::Direction> steps; };
    auto const initial = simulation.initial_state(controller, &world);
    if (!initial.unranked_body.empty() || std::any_of(initial.body.begin(), initial.body.end(), [](auto p) {
        return p.x < 0 || p.y < 0;
    })) {
        return std::nullopt;
    }
    auto queue = std::vector<Node>{{initial, {}}};
    auto budget = config::sprint_node_budget;
    auto best = std::optional<PlannedAction>{};
    for (std::size_t cursor = 0; cursor < queue.size() && budget > 0; ++cursor) {
        auto const node = queue[cursor];
        if (node.steps.size() >= config::max_sprint_steps) {
            continue;
        }
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            if (budget-- <= 0) {
                break;
            }
            auto const target = world.transition(node.state.body.front(), direction);
            auto const* tile = target ? controller.get_tile(*target) : nullptr;
            auto const* enemy = tile != nullptr ? tile->get_dragon() : nullptr;
            auto steps = node.steps;
            steps.push_back(direction);
            if (enemy != nullptr && enemy->is_head() && enemy->get_team() != controller.get_team()) {
                auto observed_length = 0;
                for (auto const& part_tile : controller.get_tiles()) {
                    auto const* part = part_tile.get_dragon();
                    observed_length += part != nullptr && part->get_id() == enemy->get_id();
                }
                auto const queen = enemy->get_id() <= 1;
                if ((queen_only && !queen) || (!queen && observed_length < controller.get_length() + 2)) {
                    continue;
                }
                auto view = controller;
                auto* attack_tile = view.get_tile(*target);
                if (attack_tile == nullptr) {
                    continue;
                }
                attack_tile->dragon_part.reset();
                if (!simulation.advance(view, node.state, direction, !node.steps.empty(), &world)) {
                    continue;
                }
                auto const score = observed_length * config::score_immediate_pearl
                    - controller.get_length() * config::score_immediate_pearl
                    - static_cast<int>(steps.size()) * config::score_sprint_tempo + (queen ? 1000000 : 0);
                if (!best || score > best->score) {
                    best = PlannedAction{};
                    best->kind = steps.size() > 1 ? ActionKind::sprint : ActionKind::move;
                    best->steps = std::move(steps);
                    best->score = score;
                    best->reason = queen ? "small helper removes fixed enemy queen"
                                         : "small helper trades for visibly larger enemy head";
                }
            } else if (auto const next = simulation.advance(controller, node.state, direction,
                                                              !node.steps.empty(), &world)) {
                queue.push_back({*next, std::move(steps)});
            }
        }
    }
    return best;
}

auto Combat::threats(unswbc::Controller const& controller, WorldModel const* world, bool long_sprints) const
    -> std::vector<ThreatAssessment> {
    auto const width = unswbc::game->width;
    auto const area = static_cast<std::size_t>(width * unswbc::game->height);
    auto result = std::vector<ThreatAssessment>(area);
    auto const index = [width](unswbc::Position p) {
        return static_cast<std::size_t>(p.y * width + p.x);
    };
    for (auto const& tile : controller.get_tiles()) {
        auto const* enemy = tile.get_dragon();
        if (enemy == nullptr || !enemy->is_head() || enemy->get_team() == controller.get_team()) {
            continue;
        }
        auto opponent = controller;
        opponent.head = *enemy;
        auto observed_length = 0;
        for (auto const& observed : controller.get_tiles()) {
            auto const* part = observed.get_dragon();
            observed_length += part != nullptr && part->get_id() == enemy->get_id();
        }
        // Partial enemy bodies give a lower bound, not the real sprint budget.
        // Price a possible second step without treating partial length as exact.
        opponent.length = std::max(3, observed_length);
        auto const unknown_extra = opponent.length - std::max(unswbc::Constants::MIN_SIZE, observed_length);
        auto const simulation = Simulation{};
        auto const initial = simulation.initial_state(opponent, world);
        auto distances = std::vector<int>(area, 0);
        auto affordable = std::vector<int>(area, 0);
        struct SearchNode { SimulationState state; int steps; bool funded; };
        auto queue = std::vector<SearchNode>{};
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            if (auto const next = simulation.advance(opponent, initial, direction, false, world)) {
                distances[index(next->body.front())] = 1;
                affordable[index(next->body.front())] = 1;
                queue.push_back({*next, 1, true});
            }
        }
        auto budget = 192;
        for (std::size_t cursor = 0; cursor < queue.size() && budget > 0; ++cursor) {
            auto const node = queue[cursor];
            if (node.steps >= (long_sprints ? 5 : 3)) {
                continue;
            }
            for (auto const direction : unswbc::Direction::get_direction_list()) {
                if (budget-- <= 0) {
                    break;
                }
                auto const next = simulation.advance(opponent, node.state, direction, true, world);
                if (!next) {
                    continue;
                }
                auto const destination = index(next->body.front());
                auto& distance = distances[destination];
                if (distance == 0) {
                    distance = node.steps + 1;
                }
                auto const funded = node.funded && (node.steps < Simulation::free_steps(
                    std::max(unswbc::Constants::MIN_SIZE, observed_length))
                    || static_cast<int>(node.state.body.size()) - unknown_extra > unswbc::Constants::MIN_SIZE);
                if (funded && affordable[destination] == 0) {
                    affordable[destination] = node.steps + 1;
                }
                queue.push_back({*next, node.steps + 1, funded});
            }
        }
        for (std::size_t i = 0; i < area; ++i) {
            auto const steps = distances[i];
            if (steps == 0) {
                continue;
            }
            auto& assessment = result[i];
            if (affordable[i] > 0 && (assessment.affordable_steps == 0
                || affordable[i] < assessment.affordable_steps)) {
                assessment.affordable_steps = affordable[i];
            }
            auto& ordered_steps = enemy->get_id() > controller.get_id()
                ? assessment.later_affordable_steps : assessment.earlier_affordable_steps;
            if (affordable[i] > 0 && (ordered_steps == 0 || affordable[i] < ordered_steps)) {
                ordered_steps = affordable[i];
            }
            if (steps == 1) {
                assessment.level = ThreatLevel::direct;
                assessment.score += config::score_enemy_head_risk;
                if (enemy->get_id() > controller.get_id()) {
                    assessment.score += config::score_enemy_head_late_risk;
                }
            } else {
                if (assessment.level == ThreatLevel::none) {
                    assessment.level = ThreatLevel::possible_sprint;
                }
                // Longer attacks must fit the observed body's segment budget
                // (or collect visible pearls). Keep them a small soft cost:
                // treating all remote routes as certain attacks starves growth.
                assessment.score += steps == 2 ? config::score_possible_enemy_sprint
                                              : config::score_long_enemy_sprint / (steps - 2);
                if (enemy->get_id() > controller.get_id() && affordable[i] > 0 && affordable[i] <= 3) {
                    assessment.score -= steps == 2 ? 15000 : 8000;
                }
            }
        }
    }
    return result;
}

auto Combat::threat_level(unswbc::Controller const& controller,
                           unswbc::Position destination, WorldModel const* world) const -> ThreatLevel {
    auto const map = threats(controller, world);
    return map[static_cast<std::size_t>(destination.y * unswbc::game->width + destination.x)].level;
}

auto Combat::destination_risk(unswbc::Controller const& controller,
                              unswbc::Position destination,
                              Role role, WorldModel const* world) const -> int {
    auto const map = threats(controller, world);
    auto score = map[static_cast<std::size_t>(destination.y * unswbc::game->width + destination.x)].score;
    if (role == Role::champion || role == Role::queen) {
        score *= config::score_champion_risk_multiplier;
    }
    return score;
}

} // namespace sudo_win

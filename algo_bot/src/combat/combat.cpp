#include "../../include/sudo_win/combat/combat.h"

#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/planner/simulation.h"
#include "../../include/sudo_win/world/world_model.h"
#include <algorithm>

namespace sudo_win {

auto Combat::favourable_trade(unswbc::Controller const& controller,
                              WorldModel const& world) const -> std::optional<PlannedAction> {
    if (controller.get_length() > 3 || controller.get_unit_count() <= 1) {
        return std::nullopt;
    }
    auto const simulation = Simulation{};
    struct Node { SimulationState state; std::vector<unswbc::Direction> steps; };
    auto queue = std::vector<Node>{{simulation.initial_state(controller, &world), {}}};
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
                if (observed_length < controller.get_length() + 2) {
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
                    - static_cast<int>(steps.size()) * config::score_sprint_tempo;
                if (!best || score > best->score) {
                    best = PlannedAction{};
                    best->kind = steps.size() > 1 ? ActionKind::sprint : ActionKind::move;
                    best->steps = std::move(steps);
                    best->score = score;
                    best->reason = "small helper trades for visibly larger enemy head";
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
            if (node.steps >= (long_sprints ? 5 : 2)) {
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
    if (role == Role::champion) {
        score *= config::score_champion_risk_multiplier;
    }
    return score;
}

} // namespace sudo_win

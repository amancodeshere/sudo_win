#include "../../include/sudo_win/splitting/splitting.h"

#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/world/world_model.h"
#include "../../include/sudo_win/planner/simulation.h"
#include "../../include/sudo_win/combat/combat.h"
#include "../../include/sudo_win/geometry/geometry.h"

#include <algorithm>

namespace sudo_win {

auto SplittingPolicy::rescue(unswbc::Controller const& controller,
                             WorldModel const& world,
                             bool certainly_trapped) const -> std::optional<PlannedAction> {
    if (!controller.can_split(unswbc::Constants::MIN_SIZE)) {
        return std::nullopt;
    }
    auto const simulation = Simulation{};
    auto const initial = simulation.initial_state(controller, &world);
    auto const complete = initial.unranked_body.empty()
        && std::none_of(initial.body.begin(), initial.body.end(), [](auto p) { return p.x < 0 || p.y < 0; });
    auto best = std::optional<PlannedAction>{};
    auto best_depth = 0;
    if (complete) {
        for (auto child_size = unswbc::Constants::MIN_SIZE;
             child_size <= controller.get_length() - unswbc::Constants::MIN_SIZE; ++child_size) {
            auto child = SimulationState{};
            child.body.assign(initial.body.rbegin(), initial.body.rbegin() + child_size);
            child.unranked_body.assign(initial.body.begin(), initial.body.end() - child_size);
            auto view = controller;
            view.get_tile(child.body.front())->dragon_part.reset();
            if (Combat{}.threat_level(view, child.body.front(), &world) == ThreatLevel::direct) {
                continue;
            }
            auto budget = 128;
            auto const depth = simulation.survival_depth(controller, child, 4, budget, &world);
            if (depth < 2 || depth <= best_depth) {
                continue;
            }
            best_depth = depth;
            best = PlannedAction{};
            best->kind = ActionKind::split;
            best->split_size = child_size;
            best->score = depth * config::score_immediate_pearl;
            best->reason = "reverse trapped tail into escaping child";
            if (depth == 4) {
                break;
            }
        }
    }
    if (!best && certainly_trapped) {
        // Movement is already fatal. A legal split keeps both snakes alive for
        // this action and gives the reversed tail an immediate escape attempt.
        // This fallback makes no claim about an unseen child's safety.
        best = PlannedAction{};
        best->kind = ActionKind::split;
        best->split_size = unswbc::Constants::MIN_SIZE;
        best->score = 0;
        best->reason = "last-resort legal split instead of certain collision";
    }
    return best;
}

auto SplittingPolicy::consider(unswbc::Controller const& controller,
                               unswbc::Game const& game,
                               Role role,
                               int reachable_area, WorldModel const* world,
                               bool enabled) const -> std::optional<PlannedAction> {
    if (!enabled || world == nullptr || game.get_round_num() >= config::split_stop_round
        || (role == Role::champion && (controller.get_unit_count() > 1 || game.get_round_num() >= 200))) {
        return std::nullopt;
    }
    if (controller.get_length() < config::split_min_length
        || controller.get_unit_count() >= std::min(config::soft_unit_cap, game.get_unit_limit())
        || reachable_area < 16) {
        return std::nullopt;
    }

    auto simulation = Simulation{};
    auto const initial = simulation.initial_state(controller, world);
    if (!initial.unranked_body.empty() || std::any_of(initial.body.begin(), initial.body.end(), [](auto p) {
        return p.x < 0 || p.y < 0;
    })) {
        return std::nullopt;
    }
    auto best = std::optional<PlannedAction>{};
    for (auto const child_size : {3, 4, controller.get_length() / 3}) {
        if (!controller.can_split(child_size) || controller.get_length() - child_size < 8) {
            continue;
        }
        auto parent = initial;
        parent.body.resize(initial.body.size() - static_cast<std::size_t>(child_size));
        auto child = SimulationState{};
        child.body.assign(initial.body.rbegin(), initial.body.rbegin() + child_size);
        parent.unranked_body = child.body;
        child.unranked_body = parent.body;
        auto view = controller;
        // Post-split heads must be treated as attackable, rather than as the
        // old body obstacles that a movement simulation would reject.
        view.get_tile(parent.body.front())->dragon_part.reset();
        view.get_tile(child.body.front())->dragon_part.reset();
        auto const combat = Combat{};
        if (combat.threat_level(view, parent.body.front(), world) != ThreatLevel::none
            || combat.threat_level(view, child.body.front(), world) != ThreatLevel::none) {
            continue;
        }
        auto const escapes = [&](SimulationState const& state) {
            auto count = 0;
            for (auto const direction : unswbc::Direction::get_direction_list()) {
                if (auto const next = simulation.advance(controller, state, direction, false, world)) {
                    auto budget = config::survival_node_budget;
                    if (simulation.survival_depth(controller, *next, config::survival_search_depth, budget, world)
                        == config::survival_search_depth) {
                        ++count;
                    }
                }
            }
            return count;
        };
        auto const parent_tiles = simulation.reachable_positions(controller, parent, world);
        auto const child_tiles = simulation.reachable_positions(controller, child, world);
        auto const parent_area = static_cast<int>(parent_tiles.size());
        auto const child_area = static_cast<int>(child_tiles.size());
        if (escapes(parent) < 2 || escapes(child) < 2
            || parent_area < static_cast<int>(parent.body.size()) + 6
            || child_area < child_size + 6) {
            continue;
        }
        auto parent_resources = 0;
        auto child_resources = 0;
        for (auto const& tile : controller.get_tiles()) {
            if (!tile.has_pearl() || tile.get_dragon() != nullptr) {
                continue;
            }
            auto const parent_distance = geometry::toroidal_manhattan(parent.body.front(), tile.get_position(),
                                                                      world->width(), world->height());
            auto const child_distance = geometry::toroidal_manhattan(child.body.front(), tile.get_position(),
                                                                     world->width(), world->height());
            if (parent_distance < child_distance && parent_distance <= 4
                && std::find(parent_tiles.begin(), parent_tiles.end(), tile.get_position()) != parent_tiles.end()) {
                ++parent_resources;
            }
            if (child_distance < parent_distance && child_distance <= 4
                && std::find(child_tiles.begin(), child_tiles.end(), tile.get_position()) != child_tiles.end()) {
                ++child_resources;
            }
        }
        if (parent_resources < 3 || child_resources < 3) {
            continue;
        }
        auto action = PlannedAction{};
        action.kind = ActionKind::split;
        action.split_size = child_size;
        action.score = parent_area * config::score_reachable_tile
                     + std::min(child_resources, 6) * (config::score_immediate_pearl / 3)
                     - child_size * config::score_split_segment_cost;
        action.reason = "safe resource-backed split";
        if (!best || action.score > best->score) {
            best = action;
        }
    }
    return best;
}

} // namespace sudo_win

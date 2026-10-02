#include "../../include/sudo_win/pathfinding/pathfinding.h"
#include "../../include/sudo_win/world/world_model.h"
#include "../../include/sudo_win/geometry/geometry.h"
#include "../../include/sudo_win/config/config.h"

#include <limits>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace sudo_win {
namespace {

[[nodiscard]] auto traversable_destination(unswbc::Controller const& controller,
                                unswbc::Position from,
                                unswbc::Direction direction,
                                WorldModel const* world) -> std::optional<unswbc::Position> {
    auto const* tile = controller.get_tile(from);
    if (tile == nullptr) {
        return std::nullopt;
    }
    auto const& edge = tile->get_edge(direction);
    if (!edge.is_passable()) {
        return std::nullopt;
    }
    auto const target = edge.is_portal() ? (world != nullptr ? world->transition(from, direction) : std::nullopt)
                                        : std::optional{from.add_dir(direction)};
    auto const* next = target ? controller.get_tile(*target) : nullptr;
    return next != nullptr && next->get_dragon() == nullptr ? target : std::nullopt;
}

} // namespace

auto Pathfinding::remembered_target(unswbc::Controller const& controller,
                                    WorldModel const& world,
                                    int round,
                                    std::optional<unswbc::Position> preferred) const
    -> std::optional<TargetRoute> {
    auto const area = static_cast<std::size_t>(world.width() * world.height());
    auto distance = std::vector<int>(area, -1);
    auto first = std::vector<unswbc::Direction>(area, unswbc::Direction::NORTH);
    auto queue = std::vector<unswbc::Position>{controller.get_position()};
    auto const index = [&world](unswbc::Position p) {
        return static_cast<std::size_t>(p.y * world.width() + p.x);
    };
    distance[index(queue.front())] = 0;
    // Claims use current visible allies and known legal routes, never stale
    // remembered occupants or geometric distance through walls/portals.
    auto ally_distance = std::vector<int>(area, -1);
    for (auto const& tile : controller.get_tiles()) {
        auto const* ally = tile.get_dragon();
        if (ally == nullptr || !ally->is_head() || ally->get_team() != controller.get_team()
            || ally->get_id() == controller.get_id()) {
            continue;
        }
        auto observed_length = 0;
        for (auto const& part_tile : controller.get_tiles()) {
            auto const* part = part_tile.get_dragon();
            observed_length += part != nullptr && part->get_id() == ally->get_id();
        }
        // Small helpers avoid races. A growing champion keeps its collection
        // priority unless an ally is visibly longer, even in a partial view.
        if (controller.get_length() > 3 && observed_length <= controller.get_length()) {
            continue;
        }
        auto claimed = std::vector<int>(area, -1);
        auto pending = std::vector<unswbc::Position>{ally->get_position()};
        claimed[index(pending.front())] = 0;
        for (std::size_t cursor = 0; cursor < pending.size() && cursor < 128U; ++cursor) {
            auto const from = pending[cursor];
            auto const steps = claimed[index(from)];
            if (steps >= 6) {
                continue;
            }
            for (auto const direction : unswbc::Direction::get_direction_list()) {
                auto const next = world.transition(from, direction);
                auto const* visible = next ? controller.get_tile(*next) : nullptr;
                if (!next || claimed[index(*next)] >= 0 || visible == nullptr
                    || visible->get_dragon() != nullptr) {
                    continue;
                }
                auto const next_index = index(*next);
                claimed[next_index] = steps + 1;
                if (ally_distance[next_index] < 0 || steps + 1 < ally_distance[next_index]) {
                    ally_distance[next_index] = steps + 1;
                }
                pending.push_back(*next);
            }
        }
    }
    auto best = std::optional<TargetRoute>{};
    auto previous = std::optional<TargetRoute>{};
    for (std::size_t cursor = 0; cursor < queue.size() && cursor < config::routing_node_budget; ++cursor) {
        auto const current = queue[cursor];
        auto const current_index = index(current);
        auto const steps = distance[current_index];
        auto const& cell = world.cell(current);
        auto const age = round - cell.last_seen_round;
        auto const pearl = cell.has_pearl && age <= config::pearl_memory_max_age;
        auto const spawning = !cell.has_pearl && cell.pearl_time >= 0 && age <= 2
                           && cell.pearl_time - age <= steps + 1;
        auto frontier = 0;
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const& edge = cell.edges[geometry::direction_index(direction)];
            if (edge.seen && edge.type == unswbc::EdgeType::EMPTY
                && !world.has_seen(current.add_dir(direction))) {
                ++frontier;
            }
        }
        if (steps > 0 && (pearl || spawning || frontier > 0)) {
            auto value = (pearl ? 24000 : spawning ? 8000 : frontier * 2400) / (steps + 1);
            if (cell.last_visited_round >= 0 && round - cell.last_visited_round < 8) {
                value /= 4;
            }
            if ((pearl || spawning) && steps > 1 && ally_distance[current_index] >= 0
                && ally_distance[current_index] < steps) {
                // Keep the resource available as a fallback; favour a different
                // collection route when an ally can reach it first.
                value /= 4;
            }
            auto const candidate = TargetRoute{current, first[current_index], steps, value, pearl || spawning};
            if (!best || candidate.value > best->value) {
                best = candidate;
            }
            if (preferred && current == *preferred) {
                previous = candidate;
            }
        }
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const target = world.transition(current, direction);
            if (!target) {
                continue;
            }
            auto const next = *target;
            auto const next_index = index(next);
            auto const* visible = controller.get_tile(next);
            // Dynamic occupancy is authoritative only in the current observation.
            if (!world.has_seen(next) || distance[next_index] >= 0
                || (visible != nullptr && visible->get_dragon() != nullptr)) {
                continue;
            }
            distance[next_index] = steps + 1;
            first[next_index] = steps == 0 ? direction : first[current_index];
            queue.push_back(next);
        }
    }
    // Keep a viable target unless an alternative is substantially better.
    if (previous && best && previous->value * 5 >= best->value * 4) {
        return previous;
    }
    return best;
}

auto Pathfinding::visible_reachable_area(unswbc::Controller const& controller,
                                         unswbc::Position start, WorldModel const* world) const -> int {
    auto frontier = std::queue<unswbc::Position>{};
    auto visited = std::unordered_set<unswbc::Position, unswbc::PositionHash>{};
    frontier.push(start);
    visited.insert(start);

    while (!frontier.empty()) {
        auto const current = frontier.front();
        frontier.pop();

        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const target = traversable_destination(controller, current, direction, world);
            if (!target) {
                continue;
            }
            auto const next = *target;
            if (visited.insert(next).second) {
                frontier.push(next);
            }
        }
    }
    return static_cast<int>(visited.size());
}

auto Pathfinding::visible_pearl_distance(unswbc::Controller const& controller,
                                         unswbc::Position start, WorldModel const* world) const -> int {
    auto frontier = std::queue<unswbc::Position>{};
    auto distance = std::unordered_map<unswbc::Position, int, unswbc::PositionHash>{};
    frontier.push(start);
    distance.emplace(start, 0);

    while (!frontier.empty()) {
        auto const current = frontier.front();
        frontier.pop();
        auto const current_distance = distance.at(current);

        auto const* tile = controller.get_tile(current);
        if (tile != nullptr && tile->has_pearl()) {
            return current_distance;
        }

        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const target = traversable_destination(controller, current, direction, world);
            if (!target) {
                continue;
            }
            auto const next = *target;
            if (!distance.contains(next)) {
                distance.emplace(next, current_distance + 1);
                frontier.push(next);
            }
        }
    }
    return std::numeric_limits<int>::max();
}

} // namespace sudo_win

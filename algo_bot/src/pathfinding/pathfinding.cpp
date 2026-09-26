#include "../../include/sudo_win/pathfinding/pathfinding.h"

#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace sudo_win {
namespace {

[[nodiscard]] auto can_traverse(unswbc::Controller const& controller,
                                unswbc::Position from,
                                unswbc::Direction direction) -> bool {
    auto const* tile = controller.get_tile(from);
    if (tile == nullptr) {
        return false;
    }
    auto const& edge = tile->get_edge(direction);
    if (!edge.is_passable() || edge.is_portal()) {
        return false;
    }
    auto const* next = controller.get_tile(from.add_dir(direction));
    return next != nullptr && next->get_dragon() == nullptr;
}

} // namespace

auto Pathfinding::visible_reachable_area(unswbc::Controller const& controller,
                                         unswbc::Position start) const -> int {
    auto frontier = std::queue<unswbc::Position>{};
    auto visited = std::unordered_set<unswbc::Position, unswbc::PositionHash>{};
    frontier.push(start);
    visited.insert(start);

    while (!frontier.empty()) {
        auto const current = frontier.front();
        frontier.pop();

        for (auto const direction : unswbc::Direction::get_direction_list()) {
            if (!can_traverse(controller, current, direction)) {
                continue;
            }
            auto const next = current.add_dir(direction);
            if (visited.insert(next).second) {
                frontier.push(next);
            }
        }
    }
    return static_cast<int>(visited.size());
}

auto Pathfinding::visible_pearl_distance(unswbc::Controller const& controller,
                                         unswbc::Position start) const -> int {
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
            if (!can_traverse(controller, current, direction)) {
                continue;
            }
            auto const next = current.add_dir(direction);
            if (!distance.contains(next)) {
                distance.emplace(next, current_distance + 1);
                frontier.push(next);
            }
        }
    }
    return std::numeric_limits<int>::max();
}

} // namespace sudo_win

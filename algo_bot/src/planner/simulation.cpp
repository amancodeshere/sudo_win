#include "../../include/sudo_win/planner/simulation.h"
#include "../../include/sudo_win/world/world_model.h"
#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/geometry/geometry.h"

#include <algorithm>

namespace sudo_win {
namespace {
[[nodiscard]] auto contains(std::vector<unswbc::Position> const& positions,
                            unswbc::Position position) -> bool {
    return std::find(positions.begin(), positions.end(), position) != positions.end();
}
} // namespace

auto Simulation::initial_state(unswbc::Controller const& controller, WorldModel const* world) const -> SimulationState {
    auto state = SimulationState{};
    state.body.resize(static_cast<std::size_t>(controller.get_length()), {-1, -1});
    state.body.front() = controller.get_position();
    for (std::size_t index = 1; index < state.body.size(); ++index) {
        auto found = false;
        for (auto const& tile : controller.get_tiles()) {
            auto const* part = tile.get_dragon();
            if (part == nullptr || part->get_id() != controller.get_id() || part->is_head()
                || contains(state.body, part->get_position())) {
                continue;
            }
            auto const ahead = world != nullptr ? world->transition(part->get_position(), part->get_dir())
                                               : std::optional{part->get_position().add_dir(part->get_dir())};
            if (ahead && *ahead == state.body[index - 1]) {
                state.body[index] = part->get_position();
                found = true;
                break;
            }
        }
        if (!found) {
            break;
        }
    }
    for (auto const& tile : controller.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (part != nullptr && part->get_id() == controller.get_id()
            && !contains(state.body, part->get_position())) {
            state.unranked_body.push_back(part->get_position());
        }
    }
    return state;
}

auto Simulation::advance(unswbc::Controller const& controller,
                          SimulationState const& state,
                          unswbc::Direction direction,
                          bool pay_sprint, WorldModel const* world) const -> std::optional<SimulationState> {
    if (state.body.empty() || (pay_sprint && state.body.size() <= unswbc::Constants::MIN_SIZE)) {
        return std::nullopt;
    }
    auto const* origin = controller.get_tile(state.body.front());
    if (origin == nullptr) {
        return std::nullopt;
    }
    auto const& edge = origin->get_edge(direction);
    if (!edge.is_passable()) {
        return std::nullopt;
    }
    auto const target = edge.is_portal()
        ? (world != nullptr ? world->transition(state.body.front(), direction) : std::nullopt)
        : std::optional{state.body.front().add_dir(direction)};
    if (!target) {
        return std::nullopt;
    }
    auto const destination = *target;
    auto const* tile = controller.get_tile(destination);
    if (tile == nullptr || contains(state.body, destination) || contains(state.unranked_body, destination)) {
        return std::nullopt;
    }
    auto const* occupant = tile->get_dragon();
    if (occupant != nullptr && occupant->get_id() != controller.get_id()) {
        return std::nullopt;
    }
    auto next = state;
    next.body.insert(next.body.begin(), destination);
    if (tile->has_pearl() && !contains(state.eaten, destination)) {
        next.eaten.push_back(destination);
        ++next.pearls;
    } else {
        next.body.pop_back();
    }
    if (pay_sprint) {
        next.body.pop_back();
    }
    return next;
}

auto Simulation::survival_depth(unswbc::Controller const& controller,
                                 SimulationState const& state,
                                 int remaining_depth,
                                 int& node_budget, WorldModel const* world) const -> int {
    if (remaining_depth == 0 || node_budget <= 0) {
        return 0;
    }
    auto best = 0;
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        --node_budget;
        if (auto const next = advance(controller, state, direction, false, world)) {
            best = std::max(best, 1 + survival_depth(controller, *next, remaining_depth - 1, node_budget, world));
            if (best == remaining_depth) {
                break;
            }
        }
        if (node_budget <= 0) {
            break;
        }
    }
    return best;
}

auto Simulation::reachable_area(unswbc::Controller const& controller,
                                 SimulationState const& state, WorldModel const* world) const -> int {
    return static_cast<int>(reachable_positions(controller, state, world).size());
}

auto Simulation::reachable_positions(unswbc::Controller const& controller,
                                      SimulationState const& state, WorldModel const* world) const
    -> std::vector<unswbc::Position> {
    auto queue = std::vector<unswbc::Position>{state.body.front()};
    for (std::size_t cursor = 0; cursor < queue.size(); ++cursor) {
        auto const current = queue[cursor];
        auto const* origin = controller.get_tile(current);
        if (origin == nullptr) {
            continue;
        }
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const& edge = origin->get_edge(direction);
            if (!edge.is_passable()) {
                continue;
            }
            auto const target = edge.is_portal() ? (world != nullptr ? world->transition(current, direction) : std::nullopt)
                                                : std::optional{current.add_dir(direction)};
            if (!target || contains(queue, *target) || contains(state.body, *target)
                || contains(state.unranked_body, *target)) {
                continue;
            }
            auto const* tile = controller.get_tile(*target);
            if (tile == nullptr || (tile->get_dragon() != nullptr
                && tile->get_dragon()->get_id() != controller.get_id())) {
                continue;
            }
            queue.push_back(*target);
        }
    }
    return queue;
}

auto Simulation::remembered_mobility(unswbc::Controller const& controller,
                                      SimulationState const& state, WorldModel const& world) const -> MobilityEstimate {
    auto const area = static_cast<std::size_t>(world.width() * world.height());
    auto blocked = std::vector<bool>(area, false);
    auto visited = std::vector<bool>(area, false);
    auto const index = [&world](unswbc::Position p) {
        return static_cast<std::size_t>(p.y * world.width() + p.x);
    };
    auto const mark_body = [&](std::vector<unswbc::Position> const& positions) {
        for (auto const p : positions) {
            if (p.x >= 0 && p.y >= 0) {
                blocked[index(p)] = true;
            }
        }
    };
    mark_body(state.body);
    mark_body(state.unranked_body);
    for (auto const& tile : controller.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (part != nullptr && part->get_id() != controller.get_id()) {
            blocked[index(tile.get_position())] = true;
        }
    }
    auto queue = std::vector<unswbc::Position>{state.body.front()};
    visited[index(queue.front())] = true;
    auto frontier = false;
    for (std::size_t cursor = 0; cursor < queue.size() && cursor < config::mobility_node_budget; ++cursor) {
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const next = world.transition(queue[cursor], direction);
            if (!next || blocked[index(*next)]) {
                continue;
            }
            if (!world.has_seen(*next)) {
                frontier = true;
                continue;
            }
            if (!visited[index(*next)]) {
                visited[index(*next)] = true;
                queue.push_back(*next);
            }
        }
    }
    // A bounded flood fill is a lower bound, never proof of a closed pocket.
    frontier = frontier || queue.size() > config::mobility_node_budget;
    return {static_cast<int>(std::min(queue.size(), static_cast<std::size_t>(config::mobility_node_budget))), frontier};
}

auto Simulation::sealed_entry_pocket(SimulationState const& state,
                                      WorldModel const& world) const -> bool {
    if (state.body.size() < 3 || !state.unranked_body.empty()
        || std::any_of(state.body.begin(), state.body.end(), [](auto p) { return p.x < 0 || p.y < 0; })) {
        return false;
    }
    // The neck seals the entrance until the tail reaches it. Ignore other
    // dragons: their movement must never be mistaken for a permanent wall.
    auto queue = std::vector<unswbc::Position>{state.body.front()};
    for (std::size_t cursor = 0; cursor < queue.size(); ++cursor) {
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const& edge = world.cell(queue[cursor]).edges[geometry::direction_index(direction)];
            if (!edge.seen) {
                return false;
            }
            if (edge.type == unswbc::EdgeType::KELP) {
                continue;
            }
            auto const next = world.transition(queue[cursor], direction);
            if (!next || !world.has_seen(*next)) {
                return false;
            }
            if (*next == state.body[1] || contains(queue, *next)) {
                continue;
            }
            // Existing tail segments inside the chamber can vacate during
            // movement; the entrance-capacity estimate does not apply.
            if (contains(state.body, *next)) {
                return false;
            }
            queue.push_back(*next);
            if (queue.size() >= state.body.size()) {
                return false;
            }
        }
    }
    return true;
}

} // namespace sudo_win

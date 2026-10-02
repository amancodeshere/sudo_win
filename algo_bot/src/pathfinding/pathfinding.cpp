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
                                    std::optional<unswbc::Position> preferred,
                                    bool protected_unit, bool portal_scout) const
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
            || ally->get_id() == controller.get_id() || controller.get_id() <= 1) {
            continue;
        }
        auto observed_length = 0;
        for (auto const& part_tile : controller.get_tiles()) {
            auto const* part = part_tile.get_dragon();
            observed_length += part != nullptr && part->get_id() == ally->get_id();
        }
        // Small helpers avoid races. A growing champion keeps its collection
        // priority unless an ally is visibly longer, even in a partial view.
        if (ally->get_id() > 1 && controller.get_length() > 3 && observed_length <= controller.get_length()) {
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
    auto remote_pearls = std::vector<bool>(area, false);
    auto claims = std::vector<int>(area, -1);
    for (auto const& report : world.reports()) {
        if (round - report.round > 4) {
            continue;
        }
        auto const p = unswbc::Position{report.x, report.y};
        auto const i = index(p);
        if (report.type == MessageType::pearl && report.round > world.cell(p).last_seen_round) {
            remote_pearls[i] = true;
        }
        auto const champion_claim = config::enable_scoring_coordination && std::any_of(world.reports().begin(),
            world.reports().end(), [&](auto const& heartbeat) {
                return heartbeat.type == MessageType::heartbeat && heartbeat.sender_id == report.sender_id
                    && round - heartbeat.round <= 2 && heartbeat.value >= std::max(8, controller.get_length() + 2);
            });
        if (report.type == MessageType::feeder && controller.get_id() > 1
            && report.sender_id != controller.get_id()
            && (report.sender_id <= 1 || report.sender_id < controller.get_id() || champion_claim)) {
            auto const remaining = std::max(0, report.value - (round - report.round));
            claims[i] = claims[i] < 0 ? remaining : std::min(claims[i], remaining);
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
        auto const claim = claims[current_index];
        auto const pearl = (cell.has_pearl && age <= config::pearl_memory_max_age) || remote_pearls[current_index];
        auto const farming = config::enable_champion_farms && (protected_unit || controller.get_id() <= 1);
        auto const remaining = cell.pearl_time - age;
        auto const spawning = !cell.has_pearl && cell.pearl_time >= 0
            && age <= (farming ? 64 : 2) && (!farming || remaining >= -2)
            && remaining <= steps + (farming ? 8 : 1);
        auto frontier = 0;
        auto portal_frontier = false;
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const& edge = cell.edges[geometry::direction_index(direction)];
            auto const* ends = edge.type == unswbc::EdgeType::PORTAL ? world.portal_endpoints(edge.portal_id) : nullptr;
            portal_frontier = portal_frontier || (config::enable_portal_routing && portal_scout && edge.seen
                && edge.type == unswbc::EdgeType::PORTAL && (ends == nullptr || ends->size() < 2U));
            if (edge.seen && edge.type == unswbc::EdgeType::EMPTY
                && !world.has_seen(current.add_dir(direction))) {
                ++frontier;
            }
        }
        if (steps > 0 && (pearl || spawning || frontier > 0 || portal_frontier)) {
            auto const wait = farming && spawning ? std::max(0, remaining - steps) : 0;
            auto value = (pearl ? 24000 : spawning ? (farming ? 12000 : 8000)
                : portal_frontier ? 12000 : frontier * 2400) / (steps + wait + 1);
            if (cell.last_visited_round >= 0 && round - cell.last_visited_round < 8
                && !(farming && (pearl || spawning))) {
                value /= 4;
            }
            if ((pearl || spawning) && steps > 1 && ally_distance[current_index] >= 0
                && ally_distance[current_index] < steps) {
                // Keep the resource available as a fallback; favour a different
                // collection route when an ally can reach it first.
                value /= 4;
            }
            if ((pearl || spawning) && claim >= 0 && claim < steps) {
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

auto Pathfinding::portal_income_route(unswbc::Controller const& controller,
                                      WorldModel const& world, int round,
                                      bool protected_unit) const -> std::optional<TargetRoute> {
    if (controller.get_unit_count() <= 1
        || controller.get_length() > (protected_unit ? (controller.get_id() <= 1 ? 8 : 12) : 4)) {
        return std::nullopt;
    }
    auto const index = [&world](unswbc::Position p) {
        return static_cast<std::size_t>(p.y * world.width() + p.x);
    };
    auto distance = std::vector<int>(static_cast<std::size_t>(world.width() * world.height()), -1);
    auto first = std::vector<unswbc::Direction>(distance.size(), unswbc::Direction::NORTH);
    auto queue = std::vector<unswbc::Position>{controller.get_position()};
    distance[index(queue.front())] = 0;
    auto best = std::optional<TargetRoute>{};
    for (std::size_t cursor = 0; cursor < queue.size() && cursor < 512U; ++cursor) {
        auto const from = queue[cursor];
        auto const steps = distance[index(from)];
        if (steps > 12) { continue; }
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const& edge = world.cell(from).edges[geometry::direction_index(direction)];
            if (!edge.seen) { continue; }
            if (edge.type == unswbc::EdgeType::PORTAL) {
                auto const visited = world.cell(from).last_visited_round;
                // The current approach tile must remain eligible for crossing.
                if (from != controller.get_position() && visited >= 0 && round - visited < 24) { continue; }
                auto const exit = world.transition(from, direction);
                auto income = 0;
                auto surveyed = false;
                for (auto const& report : world.reports()) {
                    surveyed = surveyed || (exit && report.type == MessageType::empty && report.sender_id > 1
                        && round - report.round <= 6 && report.value == edge.portal_id + 1024
                        && report.x == exit->x && report.y == exit->y);
                }
                if (exit && world.has_seen(*exit)) {
                    auto const* tile = controller.get_tile(*exit);
                    if ((tile && tile->get_dragon()) || world.cell(*exit).occupant) { continue; }
                    auto region = std::vector<unswbc::Position>{*exit};
                    auto depths = std::vector<int>{0};
                    auto onward = 0;
                    for (std::size_t i = 0; i < region.size() && i < 64U; ++i) {
                        auto const p = region[i];
                        auto const& cell = world.cell(p);
                        auto const age = round - cell.last_seen_round;
                        auto const remaining = cell.pearl_time - age;
                        income += age <= config::pearl_memory_max_age
                            && (cell.has_pearl || (cell.pearl_time >= 0 && remaining >= 0 && remaining <= 12));
                        if (depths[i] >= 4) { continue; }
                        for (auto const d : unswbc::Direction::get_direction_list()) {
                            // An exit back through the entrance is not an onward corridor.
                            auto const& e = cell.edges[geometry::direction_index(d)];
                            if (!e.seen || e.type != unswbc::EdgeType::EMPTY) { continue; }
                            auto const next = p.add_dir(d);
                            auto const* visible = controller.get_tile(next);
                            if (!world.has_seen(next) || world.cell(next).occupant
                                || (visible && visible->get_dragon())
                                || std::find(region.begin(), region.end(), next) != region.end()) { continue; }
                            onward += i == 0;
                            region.push_back(next); depths.push_back(depths[i] + 1);
                        }
                    }
                    if (onward < 2 || region.size() < static_cast<std::size_t>(controller.get_length() + 3)) { continue; }
                }
                if (!surveyed && income == 0) { continue; }
                // A mapped exhausted pocket is never treated as unexplored.
                if (exit && world.has_seen(*exit) && income == 0 && !surveyed) { continue; }
                auto const value = ((surveyed || income > 0) ? 40000 + std::min(income, 4) * 4000 : 18000)
                    / (steps + 2) + (((edge.portal_id + controller.get_id()) % 4
                        == static_cast<int>(geometry::direction_index(direction))) ? 500 : 0);
                if (!best || value > best->value) {
                    best = TargetRoute{from, steps == 0 ? direction : first[index(from)], steps + 1, value, false, true};
                }
                continue;
            }
            if (edge.type != unswbc::EdgeType::EMPTY) { continue; }
            auto const next = from.add_dir(direction);
            auto const* tile = controller.get_tile(next);
            if (!world.has_seen(next) || distance[index(next)] >= 0 || (tile && tile->get_dragon())) { continue; }
            distance[index(next)] = steps + 1;
            first[index(next)] = steps == 0 ? direction : first[index(from)];
            queue.push_back(next);
        }
    }
    return best;
}

auto Pathfinding::target_distances(unswbc::Controller const& controller, WorldModel const& world,
                                   unswbc::Position target, bool normal_edges_only) const -> std::vector<int> {
    auto const index = [&world](unswbc::Position p) { return static_cast<std::size_t>(p.y * world.width() + p.x); };
    auto distance = std::vector<int>(static_cast<std::size_t>(world.width() * world.height()), -1);
    auto incoming = std::vector<std::vector<unswbc::Position>>(distance.size());
    // Reverse actual directed transitions, including portals, rather than
    // assuming Manhattan distance or symmetric portal orientation.
    for (auto y = 0; y < world.height(); ++y) {
        for (auto x = 0; x < world.width(); ++x) {
            auto const from = unswbc::Position{x,y};
            if (!world.has_seen(from)) { continue; }
            for (auto const d : unswbc::Direction::get_direction_list()) {
                if (normal_edges_only && world.cell(from).edges[geometry::direction_index(d)].type
                    != unswbc::EdgeType::EMPTY) { continue; }
                auto const next = world.transition(from,d);
                auto const* visible = next ? controller.get_tile(*next) : nullptr;
                if (next && world.has_seen(*next) && (!visible || !visible->get_dragon())) {
                    incoming[index(*next)].push_back(from);
                }
            }
        }
    }
    auto queue = std::vector<unswbc::Position>{target};
    distance[index(target)] = 0;
    for (std::size_t cursor = 0; cursor < queue.size() && cursor < config::routing_node_budget; ++cursor) {
        for (auto const p : incoming[index(queue[cursor])]) {
            if (distance[index(p)] < 0) {
                distance[index(p)] = distance[index(queue[cursor])] + 1; queue.push_back(p);
            }
        }
    }
    return distance;
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

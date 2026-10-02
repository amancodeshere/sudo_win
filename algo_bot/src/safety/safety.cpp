#include "../../include/sudo_win/safety/safety.h"
#include "../../include/sudo_win/world/world_model.h"
#include "../../include/sudo_win/planner/simulation.h"
#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/geometry/geometry.h"
#include "../../include/sudo_win/combat/combat.h"

#include <algorithm>

namespace sudo_win {

auto Safety::unpressured_exits(unswbc::Controller const& controller, SimulationState const& after,
                                WorldModel const& world, std::vector<ThreatAssessment> const& threats) const -> int {
    auto exits = 0;
    for (auto const d : unswbc::Direction::get_direction_list()) {
        auto const next = Simulation{}.advance(controller,after,d,false,&world);
        if (!next) { continue; }
        auto const p = next->body.front();
        auto const& threat = threats[static_cast<std::size_t>(p.y * world.width() + p.x)];
        // A continuation heuristic, not a claim about an enemy's next position.
        exits += threat.level != ThreatLevel::direct
            && !(threat.affordable_steps > 0 && threat.affordable_steps <= 3);
    }
    return exits;
}

auto Safety::is_safe_standard_move(unswbc::Controller const& controller,
                                   unswbc::Direction direction, WorldModel const* world) const -> bool {
    return standard_move_reason(controller, direction, world) == SafetyReason::safe;
}

auto Safety::standard_move_reason(unswbc::Controller const& controller,
                                  unswbc::Direction direction, WorldModel const* world) const -> SafetyReason {
    auto const* origin = controller.get_tile(controller.get_position());
    if (origin == nullptr) {
        return SafetyReason::unknown_tile;
    }

    auto const& edge = origin->get_edge(direction);
    if (!edge.is_passable()) {
        return SafetyReason::wall;
    }
    auto target = controller.get_position().add_dir(direction);
    if (edge.is_portal()) {
        auto const exit = world != nullptr ? world->transition(controller.get_position(), direction) : std::nullopt;
        if (!exit) {
            return SafetyReason::unknown_portal;
        }
        target = *exit;
    }

    auto const* destination = controller.get_tile(target);
    if (destination == nullptr) {
        return SafetyReason::unknown_tile;
    }
    return destination->get_dragon() == nullptr ? SafetyReason::safe : SafetyReason::occupied;
}

auto Safety::safe_standard_moves(unswbc::Controller const& controller, WorldModel const* world) const
    -> std::vector<unswbc::Direction> {
    auto moves = std::vector<unswbc::Direction>{};
    moves.reserve(4);
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        if (is_safe_standard_move(controller, direction, world)) {
            moves.push_back(direction);
        }
    }
    return moves;
}

auto Safety::remembered_portal_escape(unswbc::Controller const& controller,
                                       WorldModel const& world, int round) const
    -> std::optional<unswbc::Direction> {
    auto const* origin = controller.get_tile(controller.get_position());
    if (origin == nullptr) {
        return std::nullopt;
    }
    auto const state = Simulation{}.initial_state(controller, &world);
    if (!state.unranked_body.empty() || std::any_of(state.body.begin(), state.body.end(), [](auto p) {
        return p.x < 0 || p.y < 0;
    })) {
        return std::nullopt;
    }
    auto const blocked = [&](unswbc::Position p) {
        return std::find(state.body.begin(), state.body.end(), p) != state.body.end()
            || world.cell(p).occupant.has_value();
    };
    auto best = std::optional<unswbc::Direction>{};
    auto best_score = -1;
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        if (!origin->get_edge(direction).is_portal()) {
            continue;
        }
        auto const exit = world.transition(controller.get_position(), direction);
        if (!exit || controller.get_tile(*exit) != nullptr || !world.has_seen(*exit) || blocked(*exit)) {
            continue;
        }
        auto const age = round - world.cell(*exit).last_seen_round;
        if (age < 0 || age > config::portal_exit_max_age) {
            continue;
        }
        auto recent_enemy = false;
        for (auto y = 0; y < world.height() && !recent_enemy; ++y) {
            for (auto x = 0; x < world.width(); ++x) {
                auto const p = unswbc::Position{x, y};
                auto const& cell = world.cell(p);
                if (cell.occupant && cell.occupant->is_head
                    && cell.occupant->team != controller.get_team().value
                    && round - cell.last_seen_round <= 2
                    && geometry::toroidal_manhattan(p, *exit, world.width(), world.height()) <= 4) {
                    recent_enemy = true;
                    break;
                }
            }
        }
        if (recent_enemy) {
            continue;
        }
        auto onward = std::vector<unswbc::Position>{};
        for (auto const d : unswbc::Direction::get_direction_list()) {
            auto const next = world.transition(*exit, d);
            if (next && world.has_seen(*next) && !blocked(*next)
                && std::find(onward.begin(), onward.end(), *next) == onward.end()) {
                onward.push_back(*next);
            }
        }
        if (onward.size() < 2) {
            continue;
        }
        auto queue = std::vector<unswbc::Position>{*exit};
        for (std::size_t cursor = 0; cursor < queue.size() && cursor < 64U; ++cursor) {
            for (auto const d : unswbc::Direction::get_direction_list()) {
                auto const next = world.transition(queue[cursor], d);
                if (next && world.has_seen(*next) && !blocked(*next)
                    && std::find(queue.begin(), queue.end(), *next) == queue.end()) {
                    queue.push_back(*next);
                }
            }
        }
        if (queue.size() < state.body.size() + 3) {
            continue;
        }
        auto const score = static_cast<int>(onward.size()) * 1000 + static_cast<int>(std::min<std::size_t>(queue.size(), 64U)) * 10 - age * 20;
        if (score > best_score) {
            best = direction;
            best_score = score;
        }
    }
    return best;
}

auto Safety::helper_portal_probe(unswbc::Controller const& controller,
                                 WorldModel const& world, int round) const
    -> std::optional<unswbc::Direction> {
    // This is explicitly uncertain exploration, reserved for expendable helpers.
    if (controller.get_id() <= 1 || controller.get_length() > 4 || controller.get_unit_count() <= 1) {
        return std::nullopt;
    }
    if (auto const known = remembered_portal_escape(controller, world, round)) {
        auto const exit = world.transition(controller.get_position(), *known);
        if (exit && round - world.cell(*exit).last_seen_round <= 4) {
            return known;
        }
    }
    auto const state = Simulation{}.initial_state(controller, &world);
    if (!state.unranked_body.empty() || std::any_of(state.body.begin(), state.body.end(), [](auto p) {
        return p.x < 0 || p.y < 0;
    })) {
        return std::nullopt;
    }
    auto const* origin = controller.get_tile(controller.get_position());
    if (origin == nullptr) {
        return std::nullopt;
    }
    if (controller.get_sonar_echoes().enemy_head > 0) {
        // Aggregate contact cannot locate an enemy or certify an exit. Defer
        // blind probing this turn; a fresh remembered candidate above is separate.
        return std::nullopt;
    }
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        if (!origin->get_edge(direction).is_portal()) {
            continue;
        }
        auto const exit = world.transition(controller.get_position(), direction);
        // Known blocked or stale exits are not reclassified as unknown.
        if (config::enable_portal_hazards && exit && world.portal_hazard(*exit,origin->get_edge(direction).get_portal_id(),round)) {
            continue;
        }
        if (!exit || (!world.has_seen(*exit) && controller.get_tile(*exit) == nullptr
            && std::find(state.body.begin(), state.body.end(), *exit) == state.body.end())) {
            return direction;
        }
    }
    return std::nullopt;
}

auto Safety::surveyed_portal_route(unswbc::Controller const& controller,
                                  WorldModel const& world, int round) const
    -> std::optional<unswbc::Direction> {
    if (controller.get_unit_count() <= 1 || controller.get_length() > (controller.get_id() <= 1 ? 8 : 12)
        || controller.get_sonar_echoes().enemy_head > 0) { return std::nullopt; }
    auto const state = Simulation{}.initial_state(controller, &world);
    if (!state.unranked_body.empty() || std::any_of(state.body.begin(), state.body.end(), [](auto p) { return p.x < 0 || p.y < 0; })) {
        return std::nullopt;
    }
    auto const* origin = controller.get_tile(controller.get_position());
    if (origin == nullptr) { return std::nullopt; }
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        auto const& edge = origin->get_edge(direction);
        if (!edge.is_portal()) { continue; }
        auto const exit = world.transition(controller.get_position(), direction);
        if (!exit || (config::enable_portal_hazards && world.portal_hazard(*exit,edge.get_portal_id(),round))
            || controller.get_tile(*exit) != nullptr || world.cell(*exit).occupant
            || std::find(state.body.begin(), state.body.end(), *exit) != state.body.end()) { continue; }
        for (auto const& report : world.reports()) {
            if (report.type == MessageType::empty && report.sender_id > 1 && round - report.round <= 1
                && report.value == edge.get_portal_id() + 1024 && report.x == exit->x && report.y == exit->y) {
                return direction;
            }
        }
    }
    return std::nullopt;
}

auto Safety::blocks_queen_escape(unswbc::Controller const& controller,
                                  SimulationState const& after, WorldModel const& world) const -> bool {
    if (controller.get_id() <= 1) { return false; }
    for (auto const& tile : controller.get_tiles()) {
        auto const* queen = tile.get_dragon();
        if (!queen || !queen->is_head() || queen->get_id() > 1 || queen->get_team() != controller.get_team()) { continue; }
        auto releasable = 0;
        auto remaining = 0;
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const target = world.transition(tile.get_position(),direction);
            auto const* next = target ? controller.get_tile(*target) : nullptr;
            if (!next) { continue; } // An unseen landing is not a certified escape.
            auto const* part = next->get_dragon();
            if (part && part->get_id() != controller.get_id()) { continue; }
            ++releasable;
            remaining += std::find(after.body.begin(),after.body.end(),*target) == after.body.end()
                && std::find(after.unranked_body.begin(),after.unranked_body.end(),*target) == after.unranked_body.end();
        }
        // Do not blame this unit for a queen trapped exclusively by others.
        if (releasable > 0 && remaining == 0) { return true; }
        if (!config::enable_queen_continuation_corridors) { continue; }
        auto const available = [&](unswbc::Position p, bool include_helper) {
            auto const* visible = controller.get_tile(p);
            if (!visible) { return false; }
            auto const* part = visible->get_dragon();
            if (part && part->get_id() != controller.get_id()) { return false; }
            return !include_helper || (std::find(after.body.begin(),after.body.end(),p) == after.body.end()
                && std::find(after.unranked_body.begin(),after.unranked_body.end(),p) == after.unranked_body.end());
        };
        auto potential_continuation = false;
        auto remaining_continuation = false;
        for (auto const first : unswbc::Direction::get_direction_list()) {
            auto const landing = world.transition(tile.get_position(),first);
            if (!landing || !available(*landing,false)) { continue; }
            for (auto const second : unswbc::Direction::get_direction_list()) {
                auto const onward = world.transition(*landing,second);
                if (!onward || *onward == tile.get_position() || *onward == *landing
                    || !available(*onward,false)) { continue; }
                potential_continuation = true;
                remaining_continuation = remaining_continuation
                    || (available(*landing,true) && available(*onward,true));
            }
        }
        // Static visible corridors only: no unseen clearance, future ally move,
        // or queen tail release is assumed. Act before the last exit is closed.
        if (potential_continuation && !remaining_continuation) { return true; }
    }
    return false;
}

auto Safety::least_bad_fallback(unswbc::Controller const& controller) const -> unswbc::Direction {
    auto best = controller.get_dir();
    auto best_rank = 7;
    // Prefer an uncertain escape over a certainly fatal wall/body collision.
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        auto const reason = standard_move_reason(controller, direction);
        auto rank = reason == SafetyReason::safe ? 0
                        : reason == SafetyReason::unknown_tile ? 1
                        : reason == SafetyReason::unknown_portal ? 2
                        : reason == SafetyReason::occupied ? 4 : 5;
        if (reason == SafetyReason::occupied) {
            auto const* tile = controller.get_tile(controller.get_position().add_dir(direction));
            auto const* part = tile != nullptr ? tile->get_dragon() : nullptr;
            if (part != nullptr && part->is_head() && part->get_id() != controller.get_id()) {
                // When every escape fails, never kill an ally as well. An
                // enemy head can at least turn the forced loss into a trade.
                rank = part->get_team() == controller.get_team() ? 6 : 3;
            }
        }
        if (rank < best_rank || (rank == best_rank && direction == controller.get_dir())) {
            best = direction;
            best_rank = rank;
        }
    }
    return best;
}

} // namespace sudo_win

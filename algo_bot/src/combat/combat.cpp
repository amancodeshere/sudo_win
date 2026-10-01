#include "../../include/sudo_win/combat/combat.h"

#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/planner/simulation.h"
#include <algorithm>

namespace sudo_win {

namespace {
[[nodiscard]] auto enemy_threat(unswbc::Controller const& controller,
                                unswbc::DragonPart const& enemy,
                                unswbc::Position destination, WorldModel const* world) -> ThreatLevel {
    auto opponent = controller;
    opponent.head = enemy;
    auto observed_length = 0;
    for (auto const& tile : controller.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (part != nullptr && part->get_id() == enemy.get_id()) {
            ++observed_length;
        }
    }
    // Enemy length is not transmitted. Price a possible short sprint as
    // uncertainty, rather than pretending observed segments are its full body.
    opponent.length = std::max(3, observed_length);
    auto const simulation = Simulation{};
    auto const initial = simulation.initial_state(opponent, world);
    auto threat = ThreatLevel::none;
    for (auto const first : unswbc::Direction::get_direction_list()) {
        auto const next = simulation.advance(opponent, initial, first, false, world);
        if (!next) {
            continue;
        }
        if (next->body.front() == destination) {
            return ThreatLevel::direct;
        }
        for (auto const second : unswbc::Direction::get_direction_list()) {
            auto const sprint = simulation.advance(opponent, *next, second, true, world);
            if (sprint && sprint->body.front() == destination) {
                threat = ThreatLevel::possible_sprint;
            }
        }
    }
    return threat;
}
} // namespace

auto Combat::threat_level(unswbc::Controller const& controller,
                           unswbc::Position destination, WorldModel const* world) const -> ThreatLevel {
    auto result = ThreatLevel::none;
    for (auto const& tile : controller.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (part == nullptr || !part->is_head() || part->get_team() == controller.get_team()) {
            continue;
        }
        auto const threat = enemy_threat(controller, *part, destination, world);
        if (threat == ThreatLevel::direct) {
            return threat;
        }
        if (threat == ThreatLevel::possible_sprint) {
            result = threat;
        }
    }
    return result;
}

auto Combat::destination_risk(unswbc::Controller const& controller,
                              unswbc::Position destination,
                              Role role, WorldModel const* world) const -> int {
    auto score = 0;
    for (auto const& tile : controller.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (part == nullptr || !part->is_head() || part->get_team() == controller.get_team()) {
            continue;
        }

        auto const threat = enemy_threat(controller, *part, destination, world);
        if (threat == ThreatLevel::direct) {
            score += config::score_enemy_head_risk;
            if (part->get_id() > controller.get_id()) {
                score += config::score_enemy_head_late_risk;
            }
        } else if (threat == ThreatLevel::possible_sprint) {
            score += config::score_possible_enemy_sprint;
        }
    }

    if (role == Role::champion) {
        score *= config::score_champion_risk_multiplier;
    }
    return score;
}

} // namespace sudo_win

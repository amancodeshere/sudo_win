#include "../../include/sudo_win/roles/roles.h"
#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/world/world_model.h"
#include <algorithm>

namespace sudo_win {

auto RoleManager::choose_role(unswbc::Controller const& controller, unswbc::Game const& game,
                              WorldModel const* world) const -> Role {
    // The starting IDs 0 and 1 are the fixed queens, regardless of colour.
    if (controller.get_id() <= 1) {
        return Role::queen;
    }
    std::erase_if(allies_, [&](auto const& ally) {
        return game.get_round_num() - ally.second.round > config::ally_estimate_max_age;
    });
    auto visible_lengths = std::unordered_map<int, int>{};
    if (world != nullptr) {
        for (auto const& report : world->reports()) {
            if (((report.type == MessageType::champion && report.sender_id <= 1)
                || (config::enable_champion_farms && report.type == MessageType::heartbeat
                    && report.sender_id > 1 && report.value >= unswbc::Constants::MIN_SIZE))
                && game.get_round_num() - report.round <= config::ally_estimate_max_age) {
                auto const existing = allies_.find(report.sender_id);
                if (existing == allies_.end() || existing->second.round <= report.round) {
                    allies_[report.sender_id] = {report.value, report.round};
                }
            }
        }
    }
    for (auto const& tile : controller.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (part != nullptr && part->get_team() == controller.get_team()) {
            ++visible_lengths[part->get_id()];
        }
    }
    for (auto const& [id, length] : visible_lengths) {
        auto const old = allies_.find(id);
        // Partial observations are lower bounds, never exact remote lengths.
        auto const recent = old != allies_.end()
                         && game.get_round_num() - old->second.round <= config::ally_estimate_max_age;
        // Remote dragons may have spent segments since the previous sighting.
        auto const decayed = recent ? old->second.length - 2 * (game.get_round_num() - old->second.round) : length;
        allies_[id] = {std::max(length, decayed), game.get_round_num()};
    }
    allies_[controller.get_id()] = {controller.get_length(), game.get_round_num()};
    auto candidate = controller.get_id();
    auto candidate_length = controller.get_length();
    if (controller.get_unit_count() > 1) {
        for (auto const& [id, estimate] : allies_) {
            if (config::enable_champion_farms && id <= 1) { continue; }
            if (game.get_round_num() - estimate.round > config::ally_estimate_max_age) {
                continue;
            }
            if (estimate.length > candidate_length || (estimate.length == candidate_length && id < candidate)) {
                candidate = id;
                candidate_length = estimate.length;
            }
        }
        auto const old = allies_.find(champion_id_);
        if (old != allies_.end() && (!config::enable_champion_farms || champion_id_ > 1)
            && game.get_round_num() - old->second.round <= config::ally_estimate_max_age
            && old->second.length + config::champion_hysteresis > candidate_length) {
            candidate = champion_id_;
        }
    }
    champion_id_ = candidate;
    if (controller.get_id() == champion_id_ && (!config::enable_champion_farms
        || controller.get_unit_count() == 1 || controller.get_length() >= (config::enable_champion_retention && game.get_round_num() >= 80 ? 4 : 8))) {
        return Role::champion;
    }
    switch (controller.get_id() % 4) {
    case 0: return Role::collector;
    case 1: return Role::scout;
    case 2: return Role::blocker;
    default: return Role::hunter;
    }
}

auto RoleManager::score_move(Role role,
                             int reachable_area,
                             int frontier_count,
                             int combat_score) const -> int {
    switch (role) {
    case Role::queen: return reachable_area * 180 + combat_score;
    case Role::champion: return reachable_area * 120 + combat_score;
    case Role::collector: return reachable_area * 40;
    case Role::scout: return frontier_count * 250;
    case Role::blocker: return combat_score / 4;
    case Role::hunter: return combat_score / 2;
    }
    return 0;
}

} // namespace sudo_win

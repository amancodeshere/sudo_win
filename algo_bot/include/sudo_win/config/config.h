#ifndef SUDO_WIN_CONFIG_CONFIG_H
#define SUDO_WIN_CONFIG_CONFIG_H

namespace sudo_win::config {

inline constexpr auto enable_sprinting = true;
inline constexpr auto enable_splitting = false;
inline constexpr auto enable_growth_splitting = false;
inline constexpr auto enable_long_sprint_threats = false;
inline constexpr auto enable_pocket_priority = false;
inline constexpr auto enable_funded_sprint_priority = false;
inline constexpr auto enable_portal_escape = true;
inline constexpr auto enable_helper_portals = true;
inline constexpr auto enable_favourable_trades = false;
inline constexpr auto enable_queen_hunting = true;
inline constexpr auto portal_exit_max_age = 16;
inline constexpr auto enable_sonar = true;
inline constexpr auto enable_indicators = false;
inline constexpr auto survival_search_depth = 6;
inline constexpr auto survival_node_budget = 512;
inline constexpr auto mobility_node_budget = 256U;
inline constexpr auto score_remembered_tile = 150;
inline constexpr auto score_closed_pocket = -160000;
inline constexpr auto max_sprint_steps = 3U;
inline constexpr auto sprint_node_budget = 84;
inline constexpr auto free_sprint_step_cap = 8;
inline constexpr auto free_sprint_node_budget = 256;
inline constexpr auto sprint_beam_per_direction = 2;
inline constexpr auto score_sprint_tempo = 1200;
inline constexpr auto routing_node_budget = 2048U;
inline constexpr auto pearl_memory_max_age = 16;
inline constexpr auto target_max_age = 8;
inline constexpr auto ally_estimate_max_age = 8;
inline constexpr auto champion_hysteresis = 2;
inline constexpr auto endgame_ramp_round = 400;
inline constexpr auto score_route_progress = 7000;
inline constexpr auto score_recent_visit = -3000;

inline constexpr auto endgame_start_round = 425;
inline constexpr auto split_min_length = 14;
inline constexpr auto split_stop_round = 300;
inline constexpr auto score_split_segment_cost = 12000;
inline constexpr auto soft_unit_cap = 4;

inline constexpr auto score_immediate_pearl = 50000;
inline constexpr auto score_reachable_tile = 1000;
inline constexpr auto score_nearby_pearl_base = 5000;
inline constexpr auto score_nearby_pearl_step = 500;
inline constexpr auto score_frontier = 180;
inline constexpr auto score_facing_continuity = 80;
inline constexpr auto score_reverse = -600;
inline constexpr auto score_future_pearl_step = 40;
inline constexpr auto score_enemy_head_risk = -120000;
inline constexpr auto score_enemy_head_late_risk = -60000;
inline constexpr auto score_possible_enemy_sprint = -30000;
inline constexpr auto score_long_enemy_sprint = -1500;
inline constexpr auto score_champion_risk_multiplier = 2;

inline constexpr auto sonar_secret = 0xD6E8FEB86659FD93ULL;
inline constexpr auto sonar_max_age = 8;

} // namespace sudo_win::config

#endif // SUDO_WIN_CONFIG_CONFIG_H

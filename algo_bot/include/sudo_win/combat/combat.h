#ifndef SUDO_WIN_COMBAT_COMBAT_H
#define SUDO_WIN_COMBAT_COMBAT_H

#include "../engine/helper.h"
#include "../types/types.h"
#include "../config/config.h"
#include <optional>

namespace sudo_win {
class WorldModel;

enum class ThreatLevel { none, possible_sprint, direct };

struct ThreatAssessment {
    ThreatLevel level = ThreatLevel::none;
    int score = 0;
    // A visible length lower bound (including pearl income) funds this route.
    // This certifies capability, not that the enemy will choose the attack.
    int affordable_steps = 0;
    int later_affordable_steps = 0;
    int earlier_affordable_steps = 0;
};

class Combat {
public:
    [[nodiscard]] auto threats(unswbc::Controller const& controller,
                               WorldModel const* world = nullptr,
                               bool long_sprints = config::enable_long_sprint_threats) const -> std::vector<ThreatAssessment>;
    [[nodiscard]] auto threat_level(unswbc::Controller const& controller,
                                    unswbc::Position destination,
                                    WorldModel const* world = nullptr) const -> ThreatLevel;
    [[nodiscard]] auto destination_risk(unswbc::Controller const& controller,
                                        unswbc::Position destination,
                                        Role role,
                                        WorldModel const* world = nullptr) const -> int;
    [[nodiscard]] auto favourable_trade(unswbc::Controller const& controller,
                                                WorldModel const& world, bool queen_only = false) const -> std::optional<PlannedAction>;
};

} // namespace sudo_win

#endif // SUDO_WIN_COMBAT_COMBAT_H

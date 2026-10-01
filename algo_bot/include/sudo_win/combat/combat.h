#ifndef SUDO_WIN_COMBAT_COMBAT_H
#define SUDO_WIN_COMBAT_COMBAT_H

#include "../engine/helper.h"
#include "../types/types.h"

namespace sudo_win {
class WorldModel;

enum class ThreatLevel { none, possible_sprint, direct };

class Combat {
public:
    [[nodiscard]] auto threat_level(unswbc::Controller const& controller,
                                    unswbc::Position destination,
                                    WorldModel const* world = nullptr) const -> ThreatLevel;
    [[nodiscard]] auto destination_risk(unswbc::Controller const& controller,
                                        unswbc::Position destination,
                                        Role role,
                                        WorldModel const* world = nullptr) const -> int;
};

} // namespace sudo_win

#endif // SUDO_WIN_COMBAT_COMBAT_H

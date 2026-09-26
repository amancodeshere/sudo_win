#ifndef SUDO_WIN_COMBAT_COMBAT_H
#define SUDO_WIN_COMBAT_COMBAT_H

#include "../engine/helper.h"
#include "../types/types.h"

namespace sudo_win {

class Combat {
public:
    [[nodiscard]] auto destination_risk(unswbc::Controller const& controller,
                                        unswbc::Position destination,
                                        Role role) const -> int;
};

} // namespace sudo_win

#endif // SUDO_WIN_COMBAT_COMBAT_H

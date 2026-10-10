#ifndef SUDO_WIN_ENDGAME_ENDGAME_H
#define SUDO_WIN_ENDGAME_ENDGAME_H

#include "../engine/helper.h"
#include "../types/types.h"

namespace sudo_win {

class Endgame {
public:
    [[nodiscard]] auto active(unswbc::Game const& game) const -> bool;
    [[nodiscard]] auto score_destination(unswbc::Controller const& controller,
                                         unswbc::Game const& game,
                                         Role role,
                                         int reachable_area,
                                         int combat_score) const -> int;
};

} // namespace sudo_win

#endif // SUDO_WIN_ENDGAME_ENDGAME_H

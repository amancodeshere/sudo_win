#ifndef SUDO_WIN_BOT_BOT_H
#define SUDO_WIN_BOT_BOT_H

#include "../planner/planner.h"
#include "../roles/roles.h"
#include "../sonar/sonar.h"
#include "../world/world_model.h"

namespace sudo_win {

class Bot {
public:
    explicit Bot(unswbc::Game const& game);
    auto execute_turn(unswbc::Controller& controller, unswbc::Game const& game) -> void;
    static auto emit_action(unswbc::Controller& controller, PlannedAction const& action) -> void;

private:
    WorldModel world_;
    Planner planner_;
    RoleManager roles_;
    SonarCodec sonar_;
    SonarScheduler sonar_scheduler_;
};

} // namespace sudo_win

#endif // SUDO_WIN_BOT_BOT_H

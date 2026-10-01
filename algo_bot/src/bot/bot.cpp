#include "../../include/sudo_win/bot/bot.h"

#include "../../include/sudo_win/config/config.h"

#include <string>
#include <exception>

namespace sudo_win {

Bot::Bot(unswbc::Game const& game)
: world_{game} {}

auto Bot::execute_turn(unswbc::Controller& controller, unswbc::Game const& game) -> void {
    auto action = PlannedAction{};
#ifndef SUDO_WIN_DEVELOPMENT
    try {
#endif
    world_.update(controller, game);

    for (auto const payload : controller.get_sonar_messages()) {
        auto const message = sonar_.decode(payload, game.get_round_num());
        if (!message.has_value()) {
            continue;
        }
    }

    auto const role = roles_.choose_role(controller, game);
    action = planner_.choose_action(controller, game, world_, role);
#ifndef SUDO_WIN_DEVELOPMENT
    } catch (std::exception const&) {
        // No action has been emitted yet. Keep the competition reply valid.
        controller.make_move(Safety{}.least_bad_fallback(controller));
        return;
    }
#endif

    if (config::enable_indicators) {
        controller.set_indicator_string(std::string{action.reason});
    }
    apply_action(controller, action);
}

auto Bot::apply_action(unswbc::Controller& controller, PlannedAction const& action) const -> void {
    if (action.kind == ActionKind::split) {
        controller.do_split(action.split_size);
        return;
    }
    if (action.kind == ActionKind::sprint && action.steps.size() > 1) {
        controller.make_moves(action.steps);
        return;
    }
    if (!action.steps.empty()) {
        controller.make_move(action.steps.front());
        return;
    }
    controller.make_move(unswbc::Direction::NORTH);
}

} // namespace sudo_win

#include "../include/sudo_win/bot/bot.h"
#include "../include/sudo_win/engine/helper.h"

auto main() -> int {
    auto [controller, game] = unswbc::init();
    auto player = sudo_win::Bot{game};

    while (unswbc::update(controller, game)) {
        player.execute_turn(controller, game);
        unswbc::end_turn();
    }
}

#include "sudo_win/combat/combat.h"

#include "sudo_win/config/config.h"

#include "../engine_fixture.h"
#include "../test_support.h"

namespace sudo_win::test {

auto run_combat_tests(TestSuite& suite) -> void {
    auto fixture = EngineFixture{};
    auto combat = Combat{};
    auto const destination = unswbc::Position{6, 5};

    fixture.controller.get_tile({7, 5})->dragon_part
        = unswbc::DragonPart{{7, 5}, 4, unswbc::Team::B, unswbc::Direction::WEST, true};

    auto const collector_risk = combat.destination_risk(fixture.controller, destination, Role::collector);
    suite.expect(collector_risk == config::score_enemy_head_risk + config::score_enemy_head_late_risk,
                 "combat applies turn-order risk for an enemy acting later");

    auto const champion_risk = combat.destination_risk(fixture.controller, destination, Role::champion);
    suite.expect(champion_risk == collector_risk * config::score_champion_risk_multiplier,
                 "champion combat risk is amplified");
}

} // namespace sudo_win::test

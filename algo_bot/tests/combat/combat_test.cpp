#include "sudo_win/combat/combat.h"

#include "sudo_win/config/config.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("combat risk scoring") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto combat = sudo_win::Combat{};
    auto const destination = unswbc::Position{6, 5};

    fixture.controller.get_tile({7, 5})->dragon_part
        = unswbc::DragonPart{{7, 5}, 4, unswbc::Team::B, unswbc::Direction::WEST, true};

    SECTION("enemy acting later adds turn-order risk") {
        auto const risk = combat.destination_risk(fixture.controller, destination, sudo_win::Role::collector);
        CHECK(risk == sudo_win::config::score_enemy_head_risk + sudo_win::config::score_enemy_head_late_risk);
    }

    SECTION("champion risk is amplified") {
        auto const collector_risk
            = combat.destination_risk(fixture.controller, destination, sudo_win::Role::collector);
        auto const champion_risk
            = combat.destination_risk(fixture.controller, destination, sudo_win::Role::champion);
        CHECK(champion_risk == collector_risk * sudo_win::config::score_champion_risk_multiplier);
    }
}

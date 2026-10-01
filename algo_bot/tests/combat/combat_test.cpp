#include "sudo_win/combat/combat.h"

#include "sudo_win/config/config.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("combat risk scoring") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto combat = sudo_win::Combat{};
    auto const destination = unswbc::Position{6, 5};

    fixture.tile({7, 5}).dragon_part
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

TEST_CASE("combat distinguishes legal threats from proximity behind walls") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto combat = sudo_win::Combat{};
    fixture.tile({7, 5}).dragon_part = unswbc::DragonPart{
        {7, 5}, 4, unswbc::Team::B, unswbc::Direction::WEST, true};
    CHECK(combat.threat_level(fixture.controller, {6, 5}) == sudo_win::ThreatLevel::direct);
    fixture.tile({7, 5}).get_edge(unswbc::Direction::WEST)
        = unswbc::Edge{false, unswbc::EdgeType::KELP};
    CHECK(combat.threat_level(fixture.controller, {6, 5}) == sudo_win::ThreatLevel::none);
    CHECK(combat.destination_risk(fixture.controller, {6, 5}, sudo_win::Role::collector) == 0);
    // Reachable in two steps, though the unreported enemy length may be too short.
    CHECK(combat.threat_level(fixture.controller, {6, 4}) == sudo_win::ThreatLevel::possible_sprint);
}

TEST_CASE("an enemy cannot cross its own body to threaten a destination") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.tile({7, 5}).dragon_part = unswbc::DragonPart{
        {7, 5}, 4, unswbc::Team::B, unswbc::Direction::EAST, true};
    fixture.tile({6, 5}).dragon_part = unswbc::DragonPart{
        {6, 5}, 4, unswbc::Team::B, unswbc::Direction::EAST, false};
    CHECK(sudo_win::Combat{}.threat_level(fixture.controller, {5, 5}) == sudo_win::ThreatLevel::none);
}

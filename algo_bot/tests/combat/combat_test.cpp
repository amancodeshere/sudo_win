#include "sudo_win/combat/combat.h"

#include "sudo_win/config/config.h"
#include "sudo_win/world/world_model.h"

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

TEST_CASE("cached threat map accumulates independent possible attacks") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.tile({7,5}).dragon_part = unswbc::DragonPart{{7,5},4,unswbc::Team::B,unswbc::Direction::WEST,true};
    fixture.tile({3,5}).dragon_part = unswbc::DragonPart{{3,5},5,unswbc::Team::B,unswbc::Direction::EAST,true};
    auto const map = sudo_win::Combat{}.threats(fixture.controller);
    CHECK(map[56].level == sudo_win::ThreatLevel::direct);
    CHECK(map[55].score == 2 * sudo_win::config::score_possible_enemy_sprint);
}

TEST_CASE("long sprint threats respect segment costs and visible pearl income") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto x = 2; x <= 5; ++x) {
        fixture.tile({x, 2}).dragon_part = unswbc::DragonPart{
            {x, 2}, 4, unswbc::Team::B, unswbc::Direction::EAST, x == 5};
    }
    auto const map = [&] { return sudo_win::Combat{}.threats(fixture.controller, nullptr, true); };
    SECTION("four observed segments afford three steps but not four") {
        auto const threat = map();
        CHECK(threat[28].level == sudo_win::ThreatLevel::possible_sprint);
        CHECK(threat[28].score == sudo_win::config::score_long_enemy_sprint);
        CHECK(threat[38].level == sudo_win::ThreatLevel::none);
    }
    SECTION("an intermediate pearl pays for a fourth step") {
        fixture.tile({6, 2}).pearl = true;
        auto const threat = map();
        CHECK(threat[38].level == sudo_win::ThreatLevel::possible_sprint);
        CHECK(threat[38].score == sudo_win::config::score_long_enemy_sprint / 2);
    }
    SECTION("two intermediate pearls fund the five-step search horizon") {
        fixture.tile({6, 2}).pearl = true;
        fixture.tile({7, 2}).pearl = true;
        auto const threat = map();
        CHECK(threat[48].level == sudo_win::ThreatLevel::possible_sprint);
        CHECK(threat[48].score == sudo_win::config::score_long_enemy_sprint / 3);
    }
    SECTION("a partial head sighting is not assumed to fund long sprints") {
        for (auto x = 2; x < 5; ++x) {
            fixture.tile({x, 2}).dragon_part.reset();
        }
        CHECK(map()[28].level == sudo_win::ThreatLevel::none);
    }
    SECTION("blocked exits do not create long attacks") {
        fixture.tile({5, 2}).get_edge(unswbc::Direction::EAST)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
        CHECK(map()[28].level == sudo_win::ThreatLevel::none);
    }
}

TEST_CASE("sprint capability separates observed funding from partial enemy uncertainty") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.tile({7,5}).dragon_part = unswbc::DragonPart{
        {7,5},4,unswbc::Team::B,unswbc::Direction::WEST,true};
    auto const threat = [&] { return sudo_win::Combat{}.threats(fixture.controller)[55]; };
    SECTION("a head alone does not prove the second step can be paid") {
        CHECK(threat().level == sudo_win::ThreatLevel::possible_sprint);
        CHECK(threat().affordable_steps == 0);
    }
    SECTION("a visible pearl funds a minimum length enemy's second step") {
        fixture.tile({6,5}).pearl = true;
        CHECK(threat().affordable_steps == 2);
    }
    SECTION("three visible body parts fund an attack without pearls") {
        fixture.tile({8,5}).dragon_part = unswbc::DragonPart{
            {8,5},4,unswbc::Team::B,unswbc::Direction::WEST,false};
        fixture.tile({8,6}).dragon_part = unswbc::DragonPart{
            {8,6},4,unswbc::Team::B,unswbc::Direction::NORTH,false};
        CHECK(threat().affordable_steps == 2);
    }
}

TEST_CASE("small helpers trade only for provably larger visible enemy heads") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.length = 2;
    fixture.controller.unit_count = 2;
    fixture.tile({5,6}).dragon_part = unswbc::DragonPart{{5,6},0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    for (auto const p : std::vector<unswbc::Position>{{7,5},{7,6},{7,7},{7,8}}) {
        fixture.tile(p).dragon_part = unswbc::DragonPart{p,4,unswbc::Team::B,unswbc::Direction::NORTH,p.y == 5};
    }
    auto const trade = [&] {
        auto world = sudo_win::WorldModel{fixture.game};
        world.update(fixture.controller,fixture.game);
        return sudo_win::Combat{}.favourable_trade(fixture.controller,world);
    };
    SECTION("minimum length cannot pay a second step without pearl income") {
        CHECK_FALSE(trade());
    }
    SECTION("an intermediate pearl funds the terminal head collision") {
        fixture.tile({6,5}).pearl = true;
        auto const attack = trade();
        REQUIRE(attack);
        CHECK(attack->kind == sudo_win::ActionKind::sprint);
        CHECK(attack->steps == std::vector<unswbc::Direction>{unswbc::Direction::EAST,unswbc::Direction::EAST});
    }
    SECTION("our last surviving unit is never sacrificed") {
        fixture.tile({6,5}).pearl = true;
        fixture.controller.unit_count = 1;
        CHECK_FALSE(trade());
    }
    SECTION("longer growing snakes are preserved") {
        fixture.tile({6,5}).pearl = true;
        fixture.controller.length = 4;
        CHECK_FALSE(trade());
    }
    SECTION("partial enemy sightings cannot justify the length trade") {
        fixture.tile({6,5}).pearl = true;
        fixture.tile({7,8}).dragon_part.reset();
        CHECK_FALSE(trade());
    }
    SECTION("walls and intermediate bodies rule out the attack") {
        fixture.tile({6,5}).pearl = true;
        fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        CHECK_FALSE(trade());
        fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{};
        fixture.tile({6,5}).dragon_part = unswbc::DragonPart{{6,5},9,unswbc::Team::A,unswbc::Direction::EAST,false};
        CHECK_FALSE(trade());
    }
}

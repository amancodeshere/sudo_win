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

TEST_CASE("threat funding preserves enemy turn order and counts free movement") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 2;
    for (auto const p : std::vector<unswbc::Position>{{8,5},{8,6},{8,7},{8,8},{7,8}}) {
        fixture.tile(p).dragon_part = unswbc::DragonPart{p,7,unswbc::Team::B,unswbc::Direction::NORTH,p == unswbc::Position{8,5}};
    }
    auto const later = sudo_win::Combat{}.threats(fixture.controller);
    CHECK(later[56].later_affordable_steps == 2);
    CHECK(later[55].later_affordable_steps == 3);
    CHECK(later[56].earlier_affordable_steps == 0);
    fixture.controller.head.dragon_id = 9;
    auto const earlier = sudo_win::Combat{}.threats(fixture.controller);
    CHECK(earlier[56].earlier_affordable_steps == 2);
    CHECK(earlier[56].later_affordable_steps == 0);
    CHECK(earlier[55].earlier_affordable_steps == 3);
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
        CHECK(threat[28].score == sudo_win::config::score_long_enemy_sprint - 8000);
        CHECK(threat[28].later_affordable_steps == 3);
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
    fixture.controller.head.dragon_id = 6;
    fixture.controller.length = 2;
    fixture.controller.unit_count = 2;
    fixture.tile({5,6}).dragon_part = unswbc::DragonPart{{5,6},6,unswbc::Team::A,unswbc::Direction::NORTH,false};
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
    SECTION("fixed queens are never sacrificed even if their role is mislabelled") {
        fixture.controller.head.dragon_id = 0;
        fixture.tile({6,5}).pearl = true;
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

TEST_CASE("small helpers target an enemy queen without requiring a length advantage") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 6;
    fixture.controller.length = 2;
    fixture.controller.unit_count = 2;
    fixture.tile({5,6}).dragon_part = unswbc::DragonPart{{5,6},6,unswbc::Team::A,unswbc::Direction::NORTH,false};
    fixture.tile({6,5}).dragon_part = unswbc::DragonPart{{6,5},1,unswbc::Team::B,unswbc::Direction::WEST,true};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto const action = sudo_win::Combat{}.favourable_trade(fixture.controller,world,true);
    REQUIRE(action);
    CHECK(action->steps == std::vector<unswbc::Direction>{unswbc::Direction::EAST});
    fixture.tile({6,5}).dragon_part->dragon_id = 7;
    world.update(fixture.controller,fixture.game);
    CHECK_FALSE(sudo_win::Combat{}.favourable_trade(fixture.controller,world,true));
}

TEST_CASE("disposable interceptors choose distinct legal queen escape routes") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 3;
    fixture.controller.length = 2;
    fixture.controller.unit_count = 3;
    fixture.tile({7,5}).dragon_part = unswbc::DragonPart{
        {7,5},1,unswbc::Team::B,unswbc::Direction::NORTH,true};
    auto const map = [&](sudo_win::Role role = sudo_win::Role::hunter) {
        auto world = sudo_win::WorldModel{fixture.game};
        world.update(fixture.controller,fixture.game);
        return sudo_win::Combat{}.interception_distances(fixture.controller,world,1,role);
    };
    SECTION("helpers spread over different exits") {
        auto const first = map();
        REQUIRE(std::count(first.begin(),first.end(),0) == 1);
        fixture.controller.head.dragon_id = 7;
        auto const second = map();
        CHECK(std::find(first.begin(),first.end(),0) - first.begin()
              != std::find(second.begin(),second.end(),0) - second.begin());
        CHECK(first[57] == -1); // The enemy head is never treated as a traversable tile.
    }
    SECTION("blocked exits and unknown cells are not assigned") {
        fixture.tile({7,5}).get_edge(unswbc::Direction::NORTH)
            = unswbc::Edge{false,unswbc::EdgeType::KELP};
        auto const distances = map();
        CHECK(distances[47] != 0);
        CHECK(distances[11] == -1);
    }
    SECTION("queens champions growing units and sole survivors retain their jobs") {
        auto const protected_map = map(sudo_win::Role::champion);
        CHECK(std::count(protected_map.begin(),protected_map.end(),0) == 0);
        fixture.controller.head.dragon_id = 0;
        auto const queen_map = map();
        CHECK(std::count(queen_map.begin(),queen_map.end(),0) == 0);
        fixture.controller.head.dragon_id = 3;
        fixture.controller.length = 5;
        auto const growing_map = map();
        CHECK(std::count(growing_map.begin(),growing_map.end(),0) == 0);
        fixture.controller.length = 2;
        fixture.controller.unit_count = 1;
        auto const sole_map = map();
        CHECK(std::count(sole_map.begin(),sole_map.end(),0) == 0);
    }
}

TEST_CASE("interception sightings expire and cannot override present observations") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 3;
    fixture.controller.length = 2;
    fixture.controller.unit_count = 2;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    world.receive_report({sudo_win::MessageType::enemy_head,1,9,7,5,1024},1);
    auto const distances = sudo_win::Combat{}.interception_distances(fixture.controller,world,1,sudo_win::Role::hunter);
    CHECK(std::count(distances.begin(),distances.end(),0) == 0);
    auto tiles = fixture.controller.vision.tiles;
    tiles.erase(std::remove_if(tiles.begin(),tiles.end(),[](auto const& tile) {
        return tile.get_position() == unswbc::Position{7,5};
    }),tiles.end());
    fixture.controller.vision = unswbc::Vision{std::move(tiles)};
    auto const fresh = sudo_win::Combat{}.interception_distances(fixture.controller,world,2,sudo_win::Role::hunter);
    CHECK(std::count(fresh.begin(),fresh.end(),0) == 1);
    auto const expired = sudo_win::Combat{}.interception_distances(fixture.controller,world,4,sudo_win::Role::hunter);
    CHECK(std::count(expired.begin(),expired.end(),0) == 0);
}

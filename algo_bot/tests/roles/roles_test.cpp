#include "sudo_win/roles/roles.h"
#include "sudo_win/world/world_model.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("role assignment and scoring") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto roles = sudo_win::RoleManager{};

    SECTION("fixed queen IDs keep their role regardless of colour or length") {
        CHECK(roles.choose_role(fixture.controller, fixture.game) == sudo_win::Role::queen);
        fixture.controller.head.dragon_id = 1;
        fixture.controller.head.team = unswbc::Team::B;
        fixture.controller.unit_count = 4;
        CHECK(roles.choose_role(fixture.controller, fixture.game) == sudo_win::Role::queen);
    }
    SECTION("a sole non-queen survivor retains the secondary champion role") {
        fixture.controller.head.dragon_id = 7;
        fixture.controller.head.team = unswbc::Team::B;
        CHECK(roles.choose_role(fixture.controller, fixture.game) == sudo_win::Role::champion);
    }

    SECTION("non-champion IDs receive deterministic roles") {
        fixture.controller.head.dragon_id = 5;
        fixture.controller.unit_count = 2;
        for (auto const position : {unswbc::Position{6, 5}, unswbc::Position{6, 6},
                                    unswbc::Position{6, 7}, unswbc::Position{7, 7}}) {
            fixture.tile(position).dragon_part = unswbc::DragonPart{
                position, 2, unswbc::Team::A, unswbc::Direction::NORTH, position == unswbc::Position{6, 5}};
        }
        CHECK(roles.choose_role(fixture.controller, fixture.game) == sudo_win::Role::scout);
    }

    SECTION("scouts value exploration more than collectors") {
        CHECK(roles.score_move(sudo_win::Role::scout, 0, 3, 0)
              > roles.score_move(sudo_win::Role::collector, 0, 3, 0));
    }
}

TEST_CASE("champion estimates expire and do not oscillate on small length changes") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 7;
    fixture.controller.unit_count = 2;
    for (auto const position : {unswbc::Position{6, 5}, unswbc::Position{6, 6},
                                unswbc::Position{6, 7}, unswbc::Position{7, 7}, unswbc::Position{7, 6}}) {
        fixture.tile(position).dragon_part = unswbc::DragonPart{
            position, 2, unswbc::Team::A, unswbc::Direction::NORTH, position == unswbc::Position{6, 5}};
    }
    auto roles = sudo_win::RoleManager{};
    CHECK(roles.choose_role(fixture.controller, fixture.game) != sudo_win::Role::champion);
    fixture.controller.length = 6;
    CHECK(roles.choose_role(fixture.controller, fixture.game) != sudo_win::Role::champion);
    fixture.controller.length = 8;
    CHECK(roles.choose_role(fixture.controller, fixture.game) == sudo_win::Role::champion);
    fixture.controller.length = 3;
    fixture.game.round_num = 20;
    for (auto& tile : fixture.controller.vision.tiles) {
        tile.dragon_part.reset();
    }
    CHECK(roles.choose_role(fixture.controller, fixture.game) != sudo_win::Role::champion);
}

TEST_CASE("a separate helper champion persists beside a queen and learns fresh remote lengths") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 7;
    fixture.controller.length = 8;
    fixture.controller.unit_count = 3;
    fixture.tile({6,5}).dragon_part = unswbc::DragonPart{{6,5},0,unswbc::Team::A,unswbc::Direction::NORTH,true};
    fixture.tile({6,6}).dragon_part = unswbc::DragonPart{{6,6},0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    fixture.tile({6,7}).dragon_part = unswbc::DragonPart{{6,7},0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    fixture.tile({7,7}).dragon_part = unswbc::DragonPart{{7,7},0,unswbc::Team::A,unswbc::Direction::WEST,false};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto roles = sudo_win::RoleManager{};
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) == sudo_win::Role::champion);
    world.receive_report({sudo_win::MessageType::heartbeat,0,4,0,0,12},0);
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) != sudo_win::Role::champion);
    CHECK_FALSE(world.has_seen({0,0}));
    fixture.game.round_num = 9;
    world.update(fixture.controller,fixture.game);
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) == sudo_win::Role::champion);
}

TEST_CASE("a four segment secondary scorer is protected after early expansion") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    fixture.controller.unit_count = 3;
    auto roles = sudo_win::RoleManager{};
    fixture.controller.length = 3;
    CHECK(roles.choose_role(fixture.controller,fixture.game) == sudo_win::Role::collector);
    fixture.controller.length = 4;
    fixture.game.round_num = 79;
    CHECK(roles.choose_role(fixture.controller,fixture.game) == sudo_win::Role::collector);
    fixture.game.round_num = 80;
    CHECK(roles.choose_role(fixture.controller,fixture.game) == sudo_win::Role::champion);
    fixture.controller.length = 6;
    CHECK(roles.choose_role(fixture.controller,fixture.game) == sudo_win::Role::champion);
}

TEST_CASE("election hysteresis cannot borrow a teammate's length for champion eligibility") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    fixture.controller.length = 3;
    fixture.controller.unit_count = 3;
    fixture.game.round_num = 100;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto roles = sudo_win::RoleManager{};
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) == sudo_win::Role::collector);
    world.receive_report({sudo_win::MessageType::heartbeat,100,8,0,0,4},100);
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) == sudo_win::Role::collector);
}

TEST_CASE("champion coordination protects durable scorers without freezing rich helper swarms") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    fixture.controller.length = 4;
    fixture.controller.unit_count = 12;
    fixture.game.round_num = 100;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto roles = sudo_win::RoleManager{};
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) == sudo_win::Role::collector);
    fixture.controller.length = 8;
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) == sudo_win::Role::champion);
    world.receive_report({sudo_win::MessageType::heartbeat,100,8,0,0,12},100);
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) != sudo_win::Role::champion);
    fixture.game.round_num = 103;
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) == sudo_win::Role::champion);
    fixture.controller.length = 4;
    fixture.controller.unit_count = 1;
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) == sudo_win::Role::champion);
}

TEST_CASE("mature scorers retain protection beside a longer elected champion") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 7;
    fixture.controller.unit_count = 10;
    fixture.game.round_num = 100;
    fixture.controller.length = 20;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    world.receive_report({sudo_win::MessageType::heartbeat,100,4,0,0,24},100);
    auto roles = sudo_win::RoleManager{};
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) == sudo_win::Role::champion);
    fixture.controller.length = 12;
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) != sudo_win::Role::champion);
    fixture.game.round_num = 200;
    world.receive_report({sudo_win::MessageType::heartbeat,200,4,0,0,24},200);
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) == sudo_win::Role::champion);
    fixture.controller.length = 8;
    CHECK(roles.choose_role(fixture.controller,fixture.game,&world) != sudo_win::Role::champion);
}

#include "sudo_win/roles/roles.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("role assignment and scoring") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto roles = sudo_win::RoleManager{};

    SECTION("the only living friendly dragon is champion regardless of colour or ID") {
        CHECK(roles.choose_role(fixture.controller, fixture.game) == sudo_win::Role::champion);
        fixture.controller.head.dragon_id = 7;
        fixture.controller.head.team = unswbc::Team::B;
        CHECK(roles.choose_role(fixture.controller, fixture.game) == sudo_win::Role::champion);
    }

    SECTION("non-champion IDs receive deterministic roles") {
        fixture.controller.head.dragon_id = 1;
        fixture.controller.unit_count = 2;
        for (auto const position : {unswbc::Position{6, 5}, unswbc::Position{6, 6},
                                    unswbc::Position{6, 7}, unswbc::Position{7, 7}}) {
            fixture.tile(position).dragon_part = unswbc::DragonPart{
                position, 0, unswbc::Team::A, unswbc::Direction::NORTH, position == unswbc::Position{6, 5}};
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
    CHECK(roles.choose_role(fixture.controller, fixture.game) == sudo_win::Role::champion);
}

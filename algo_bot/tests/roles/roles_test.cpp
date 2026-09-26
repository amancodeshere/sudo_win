#include "sudo_win/roles/roles.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("role assignment and scoring") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto roles = sudo_win::RoleManager{};

    SECTION("dragon zero begins as champion") {
        CHECK(roles.choose_role(fixture.controller, fixture.game) == sudo_win::Role::champion);
    }

    SECTION("non-champion IDs receive deterministic roles") {
        fixture.controller.head.dragon_id = 1;
        CHECK(roles.choose_role(fixture.controller, fixture.game) == sudo_win::Role::scout);
    }

    SECTION("scouts value exploration more than collectors") {
        CHECK(roles.score_move(sudo_win::Role::scout, 0, 3, 0)
              > roles.score_move(sudo_win::Role::collector, 0, 3, 0));
    }
}

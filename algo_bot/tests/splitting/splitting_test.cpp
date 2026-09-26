#include "sudo_win/splitting/splitting.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("stable splitting policy") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.length = 20;
    auto policy = sudo_win::SplittingPolicy{};

    SECTION("splitting remains disabled") {
        CHECK_FALSE(policy.consider(fixture.controller, fixture.game, sudo_win::Role::collector, 49).has_value());
    }
}

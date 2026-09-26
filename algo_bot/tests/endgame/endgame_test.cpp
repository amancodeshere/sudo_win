#include "sudo_win/endgame/endgame.h"

#include "sudo_win/config/config.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("endgame phase and survival weighting") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto endgame = sudo_win::Endgame{};

    SECTION("phase activates at the configured threshold") {
        fixture.game.round_num = sudo_win::config::endgame_start_round - 1;
        CHECK_FALSE(endgame.active(fixture.game));

        fixture.game.round_num = sudo_win::config::endgame_start_round;
        CHECK(endgame.active(fixture.game));
    }

    SECTION("champion receives stronger survival weighting") {
        fixture.game.round_num = sudo_win::config::endgame_start_round;
        auto const collector_score = endgame.score_destination(fixture.controller,
                                                               fixture.game,
                                                               sudo_win::Role::collector,
                                                               10,
                                                               -100);
        auto const champion_score = endgame.score_destination(fixture.controller,
                                                              fixture.game,
                                                              sudo_win::Role::champion,
                                                              10,
                                                              -100);
        CHECK(champion_score == collector_score * 2);
    }
}

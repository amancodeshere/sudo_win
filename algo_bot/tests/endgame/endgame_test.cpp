#include "sudo_win/endgame/endgame.h"

#include "sudo_win/config/config.h"

#include "../engine_fixture.h"
#include "../test_support.h"

namespace sudo_win::test {

auto run_endgame_tests(TestSuite& suite) -> void {
    auto fixture = EngineFixture{};
    auto endgame = Endgame{};

    fixture.game.round_num = config::endgame_start_round - 1;
    suite.expect(!endgame.active(fixture.game), "endgame remains inactive before its threshold");

    fixture.game.round_num = config::endgame_start_round;
    suite.expect(endgame.active(fixture.game), "endgame activates at its threshold");

    auto const collector_score = endgame.score_destination(fixture.controller,
                                                           fixture.game,
                                                           Role::collector,
                                                           10,
                                                           -100);
    auto const champion_score = endgame.score_destination(fixture.controller,
                                                          fixture.game,
                                                          Role::champion,
                                                          10,
                                                          -100);
    suite.expect(champion_score == collector_score * 2,
                 "champion receives stronger endgame survival weighting");
}

} // namespace sudo_win::test

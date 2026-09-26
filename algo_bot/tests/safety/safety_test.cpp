#include "sudo_win/safety/safety.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("standard movement safety") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto safety = sudo_win::Safety{};

    SECTION("all empty adjacent tiles are safe") {
        CHECK(safety.safe_standard_moves(fixture.controller).size() == 4);
    }

    SECTION("kelp is rejected") {
        auto* origin = fixture.controller.get_tile({5, 5});
        origin->get_edge(unswbc::Direction::NORTH) = unswbc::Edge{true, unswbc::EdgeType::KELP};
        CHECK_FALSE(safety.is_safe_standard_move(fixture.controller, unswbc::Direction::NORTH));
    }

    SECTION("unknown portal exits are rejected") {
        auto* origin = fixture.controller.get_tile({5, 5});
        origin->get_edge(unswbc::Direction::SOUTH) = unswbc::Edge{true, unswbc::EdgeType::PORTAL, 7};
        CHECK_FALSE(safety.is_safe_standard_move(fixture.controller, unswbc::Direction::SOUTH));
    }

    SECTION("occupied destinations are rejected") {
        auto* east = fixture.controller.get_tile({6, 5});
        east->dragon_part = unswbc::DragonPart{{6, 5}, 1, unswbc::Team::B, unswbc::Direction::WEST, false};
        CHECK_FALSE(safety.is_safe_standard_move(fixture.controller, unswbc::Direction::EAST));
    }
}

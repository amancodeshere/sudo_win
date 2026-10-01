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
        auto& origin = fixture.tile({5, 5});
        origin.get_edge(unswbc::Direction::NORTH) = unswbc::Edge{true, unswbc::EdgeType::KELP};
        CHECK_FALSE(safety.is_safe_standard_move(fixture.controller, unswbc::Direction::NORTH));
    }

    SECTION("unknown portal exits are rejected") {
        auto& origin = fixture.tile({5, 5});
        origin.get_edge(unswbc::Direction::SOUTH) = unswbc::Edge{true, unswbc::EdgeType::PORTAL, 7};
        CHECK_FALSE(safety.is_safe_standard_move(fixture.controller, unswbc::Direction::SOUTH));
    }

    SECTION("occupied destinations are rejected") {
        auto& east = fixture.tile({6, 5});
        east.dragon_part = unswbc::DragonPart{{6, 5}, 1, unswbc::Team::B, unswbc::Direction::WEST, false};
        CHECK_FALSE(safety.is_safe_standard_move(fixture.controller, unswbc::Direction::EAST));
    }
}

TEST_CASE("fallback ranks safe and uncertain escapes ahead of fatal moves") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto safety = sudo_win::Safety{};
    fixture.tile({5, 4}).dragon_part = unswbc::DragonPart{
        {5, 4}, 0, unswbc::Team::A, unswbc::Direction::SOUTH, false};
    CHECK(safety.standard_move_reason(fixture.controller, unswbc::Direction::NORTH)
          == sudo_win::SafetyReason::occupied);
    CHECK(safety.least_bad_fallback(fixture.controller) != unswbc::Direction::NORTH);
    for (auto const direction : {unswbc::Direction::EAST, unswbc::Direction::SOUTH}) {
        fixture.tile({5, 5}).get_edge(direction)
            = unswbc::Edge{false, unswbc::EdgeType::KELP};
    }
    fixture.tile({5, 5}).get_edge(unswbc::Direction::WEST)
        = unswbc::Edge{false, unswbc::EdgeType::PORTAL, 1};
    CHECK(safety.least_bad_fallback(fixture.controller) == unswbc::Direction::WEST);
}

TEST_CASE("safety resolves wrapped destinations including own body") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.position = {0, 5};
    fixture.controller.vision.tiles.emplace_back(unswbc::Position{0, 5});
    fixture.controller.vision.tiles.emplace_back(unswbc::Position{9, 5});
    fixture.controller.vision = unswbc::Vision{fixture.controller.vision.tiles};
    auto safety = sudo_win::Safety{};
    CHECK(safety.is_safe_standard_move(fixture.controller, unswbc::Direction::WEST));
    fixture.tile({9, 5}).dragon_part = unswbc::DragonPart{
        {9, 5}, 0, unswbc::Team::A, unswbc::Direction::EAST, false};
    CHECK_FALSE(safety.is_safe_standard_move(fixture.controller, unswbc::Direction::WEST));
}

TEST_CASE("fatal fallback protects allied heads and prefers enemy trades") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dir = unswbc::Direction::EAST;
    fixture.tile({6,5}).dragon_part = unswbc::DragonPart{{6,5},2,unswbc::Team::A,unswbc::Direction::WEST,true};
    for (auto const direction : {unswbc::Direction::NORTH, unswbc::Direction::SOUTH, unswbc::Direction::WEST}) {
        fixture.tile({5,5}).get_edge(direction) = unswbc::Edge{false,unswbc::EdgeType::KELP};
    }
    CHECK(sudo_win::Safety{}.least_bad_fallback(fixture.controller) != unswbc::Direction::EAST);
    fixture.tile({5,5}).get_edge(unswbc::Direction::NORTH) = unswbc::Edge{false,unswbc::EdgeType::EMPTY};
    fixture.tile({5,4}).dragon_part = unswbc::DragonPart{{5,4},3,unswbc::Team::B,unswbc::Direction::SOUTH,true};
    CHECK(sudo_win::Safety{}.least_bad_fallback(fixture.controller) == unswbc::Direction::NORTH);
}

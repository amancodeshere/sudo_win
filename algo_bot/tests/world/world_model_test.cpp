#include "sudo_win/world/world_model.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("world model observations") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game.round_num = 12;

    auto* east = fixture.controller.get_tile({6, 5});
    east->pearl = true;
    east->pearl_time = 4;
    east->get_edge(unswbc::Direction::EAST) = unswbc::Edge{false, unswbc::EdgeType::PORTAL, 9};

    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);

    SECTION("visible cell state is remembered") {
        auto const& remembered = world.cell({6, 5});
        CHECK(remembered.seen);
        CHECK(remembered.last_seen_round == 12);
        CHECK(remembered.has_pearl);
        CHECK(remembered.pearl_time == 4);
    }

    SECTION("unique portal endpoints are remembered") {
        auto const* endpoints = world.portal_endpoints(9);
        REQUIRE(endpoints != nullptr);
        CHECK(endpoints->size() == 1);
    }

    SECTION("vision boundaries are exploration frontiers") {
        CHECK(world.unseen_neighbour_count({2, 2}) == 2);
    }
}

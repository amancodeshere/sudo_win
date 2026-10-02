#include "sudo_win/world/world_model.h"
#include "sudo_win/safety/safety.h"
#include "sudo_win/planner/simulation.h"
#include "sudo_win/pathfinding/pathfinding.h"
#include <algorithm>

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("world model observations") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game.round_num = 12;

    auto& east = fixture.tile({6, 5});
    east.pearl = true;
    east.pearl_time = 4;
    east.get_edge(unswbc::Direction::EAST) = unswbc::Edge{false, unswbc::EdgeType::PORTAL, 9};

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

TEST_CASE("paired portal boundaries preserve heading and deduplicate both sides") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto const position : {unswbc::Position{5, 5}, unswbc::Position{7, 7}}) {
        fixture.tile(position).get_edge(unswbc::Direction::EAST)
            = unswbc::Edge{false, unswbc::EdgeType::PORTAL, 9};
        fixture.tile(position.add_dir(unswbc::Direction::EAST)).get_edge(unswbc::Direction::WEST)
            = unswbc::Edge{false, unswbc::EdgeType::PORTAL, 9};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    REQUIRE(world.portal_endpoints(9));
    CHECK(world.portal_endpoints(9)->size() == 2);
    CHECK(world.transition({5, 5}, unswbc::Direction::EAST) == unswbc::Position{8, 7});
    CHECK(world.transition({6, 5}, unswbc::Direction::WEST) == unswbc::Position{7, 7});
    CHECK(world.transition({7, 7}, unswbc::Direction::EAST) == unswbc::Position{6, 5});
    CHECK(world.transition({8, 7}, unswbc::Direction::WEST) == unswbc::Position{5, 5});
    auto safety = sudo_win::Safety{};
    auto simulation = sudo_win::Simulation{};

    SECTION("a visible empty exit can be crossed and scored as the actual destination") {
        CHECK(safety.is_safe_standard_move(fixture.controller, unswbc::Direction::EAST, &world));
        fixture.tile({8, 7}).pearl = true;
        auto const next = simulation.advance(fixture.controller, simulation.initial_state(fixture.controller, &world),
                                              unswbc::Direction::EAST, false, &world);
        REQUIRE(next);
        CHECK(next->body.front() == unswbc::Position{8, 7});
        CHECK(next->pearls == 1);
        CHECK(sudo_win::Pathfinding{}.visible_pearl_distance(fixture.controller, {5, 5}, &world) == 1);
    }
    SECTION("occupied exits remain fatal") {
        fixture.tile({8, 7}).dragon_part = unswbc::DragonPart{
            {8, 7}, 2, unswbc::Team::B, unswbc::Direction::WEST, false};
        CHECK(safety.standard_move_reason(fixture.controller, unswbc::Direction::EAST, &world)
              == sudo_win::SafetyReason::occupied);
    }
    SECTION("remembered occupancy does not certify an unseen exit") {
        auto tiles = fixture.controller.vision.tiles;
        tiles.erase(std::remove_if(tiles.begin(), tiles.end(), [](auto const& tile) {
            return tile.get_position() == unswbc::Position{8, 7};
        }), tiles.end());
        fixture.controller.vision = unswbc::Vision{std::move(tiles)};
        CHECK(safety.standard_move_reason(fixture.controller, unswbc::Direction::EAST, &world)
              == sudo_win::SafetyReason::unknown_tile);
    }
    SECTION("body reconstruction follows portal links rather than adjacent coordinates") {
        fixture.controller.head.position = {8, 7};
        fixture.tile({5, 5}).dragon_part = unswbc::DragonPart{
            {5, 5}, 0, unswbc::Team::A, unswbc::Direction::EAST, false};
        fixture.tile({5, 6}).dragon_part = unswbc::DragonPart{
            {5, 6}, 0, unswbc::Team::A, unswbc::Direction::NORTH, false};
        auto const state = simulation.initial_state(fixture.controller, &world);
        CHECK(state.body[1] == unswbc::Position{5, 5});
        CHECK(state.body[2] == unswbc::Position{5, 6});
        CHECK_FALSE(simulation.advance(fixture.controller, state, unswbc::Direction::WEST, false, &world));
    }
}

TEST_CASE("horizontal portal crossings choose the heading side of the partner") {
    auto fixture = sudo_win::test::EngineFixture{};
    for (auto const position : {unswbc::Position{5, 5}, unswbc::Position{7, 7}}) {
        fixture.tile(position).get_edge(unswbc::Direction::NORTH)
            = unswbc::Edge{true, unswbc::EdgeType::PORTAL, 9};
        fixture.tile(position.add_dir(unswbc::Direction::NORTH)).get_edge(unswbc::Direction::SOUTH)
            = unswbc::Edge{true, unswbc::EdgeType::PORTAL, 9};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    CHECK(world.transition({5, 5}, unswbc::Direction::NORTH) == unswbc::Position{7, 6});
    CHECK(world.transition({5, 4}, unswbc::Direction::SOUTH) == unswbc::Position{7, 7});
}

TEST_CASE("confirmed own movement preserves body order outside current vision") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game.round_num = 0;
    for (auto const p : std::vector<unswbc::Position>{{5,6},{5,7}}) {
        fixture.tile(p).dragon_part = unswbc::DragonPart{p,0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto action = sudo_win::PlannedAction{};
    action.steps = {unswbc::Direction::EAST};
    world.remember_action(fixture.controller,fixture.game,action);
    fixture.controller.head.position = {6,5};
    fixture.game.round_num = 1;
    fixture.tile({5,5}).dragon_part = unswbc::DragonPart{{5,5},0,unswbc::Team::A,unswbc::Direction::EAST,false};
    fixture.tile({5,7}).dragon_part.reset();
    auto tiles = fixture.controller.vision.tiles;
    tiles.erase(std::remove_if(tiles.begin(),tiles.end(),[](auto const& tile) {
        return tile.get_position() == unswbc::Position{5,6};
    }),tiles.end());
    fixture.controller.vision = unswbc::Vision{std::move(tiles)};
    SECTION("a hidden tail keeps its known rank") {
        world.update(fixture.controller,fixture.game);
        auto state = sudo_win::Simulation{}.initial_state(fixture.controller,&world);
        CHECK(state.body == std::vector<unswbc::Position>{{6,5},{5,5},{5,6}});
        CHECK(state.unranked_body.empty());
    }
    SECTION("conflicting visible occupancy invalidates the prediction") {
        fixture.tile({5,5}).dragon_part.reset();
        world.update(fixture.controller,fixture.game);
        auto state = sudo_win::Simulation{}.initial_state(fixture.controller,&world);
        CHECK(state.body[1] == unswbc::Position{-1,-1});
    }
    SECTION("a skipped observation cannot confirm a previous action") {
        fixture.game.round_num = 2;
        world.update(fixture.controller,fixture.game);
        auto state = sudo_win::Simulation{}.initial_state(fixture.controller,&world);
        CHECK(state.body[2] == unswbc::Position{-1,-1});
    }
}

TEST_CASE("a portal landing confirms an unseen body link without guessing the partner") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game.round_num = 0;
    for (auto const p : std::vector<unswbc::Position>{{5,6},{5,7}}) {
        fixture.tile(p).dragon_part = unswbc::DragonPart{p,0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    }
    fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto action = sudo_win::PlannedAction{};
    action.steps = {unswbc::Direction::EAST};
    world.remember_action(fixture.controller,fixture.game,action);
    fixture.controller.head.position = {0,0};
    fixture.controller.vision = unswbc::Vision{std::vector<unswbc::Tile>{unswbc::Tile{{0,0}}}};
    fixture.game.round_num = 1;
    world.update(fixture.controller,fixture.game);
    auto state = sudo_win::Simulation{}.initial_state(fixture.controller,&world);
    CHECK(state.body == std::vector<unswbc::Position>{{0,0},{5,5},{5,6}});
    REQUIRE(world.portal_endpoints(9));
    CHECK(world.portal_endpoints(9)->size() == 1);
    CHECK_FALSE(world.transition({5,5},unswbc::Direction::EAST));
}

TEST_CASE("sprint and split body predictions are checked against the next observation") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game.round_num = 0;
    fixture.controller.length = 4;
    for (auto const p : std::vector<unswbc::Position>{{5,6},{5,7},{5,8}}) {
        fixture.tile(p).dragon_part = unswbc::DragonPart{p,0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    SECTION("growth is confirmed using actual length after a single step") {
        auto action = sudo_win::PlannedAction{};
        action.steps = {unswbc::Direction::EAST};
        world.remember_action(fixture.controller,fixture.game,action);
        fixture.controller.head.position = {6,5};
        fixture.controller.length = 5;
        fixture.controller.vision = unswbc::Vision{std::vector<unswbc::Tile>{unswbc::Tile{{6,5}}}};
        fixture.game.round_num = 1;
        world.update(fixture.controller,fixture.game);
        CHECK(sudo_win::Simulation{}.initial_state(fixture.controller,&world).body
              == std::vector<unswbc::Position>{{6,5},{5,5},{5,6},{5,7},{5,8}});
    }
    SECTION("paid steps release the correct number of tail segments") {
        auto action = sudo_win::PlannedAction{};
        action.kind = sudo_win::ActionKind::sprint;
        action.steps = {unswbc::Direction::EAST,unswbc::Direction::EAST};
        world.remember_action(fixture.controller,fixture.game,action);
        fixture.controller.head.position = {7,5};
        fixture.controller.length = 3;
        fixture.controller.vision = unswbc::Vision{std::vector<unswbc::Tile>{unswbc::Tile{{7,5}}}};
        fixture.game.round_num = 1;
        world.update(fixture.controller,fixture.game);
        CHECK(sudo_win::Simulation{}.initial_state(fixture.controller,&world).body
              == std::vector<unswbc::Position>{{7,5},{6,5},{5,5}});
    }
    SECTION("a split retains the parent's prefix, not the reversed child") {
        auto action = sudo_win::PlannedAction{};
        action.kind = sudo_win::ActionKind::split;
        action.split_size = 2;
        world.remember_action(fixture.controller,fixture.game,action);
        fixture.controller.length = 2;
        fixture.controller.vision = unswbc::Vision{std::vector<unswbc::Tile>{unswbc::Tile{{5,5}}}};
        fixture.game.round_num = 1;
        world.update(fixture.controller,fixture.game);
        CHECK(sudo_win::Simulation{}.initial_state(fixture.controller,&world).body
              == std::vector<unswbc::Position>{{5,5},{5,6}});
    }
}

TEST_CASE("unknown initial body ranks are learned through confirmed movement") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game.round_num = 0;
    fixture.controller.vision = unswbc::Vision{std::vector<unswbc::Tile>{unswbc::Tile{{5,5}}}};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    CHECK(sudo_win::Simulation{}.initial_state(fixture.controller,&world).body.back() == unswbc::Position{-1,-1});
    for (auto round = 1; round <= 3; ++round) {
        auto action = sudo_win::PlannedAction{};
        action.steps = {unswbc::Direction::EAST};
        world.remember_action(fixture.controller,fixture.game,action);
        fixture.controller.head.position = {5+round,5};
        fixture.controller.vision = unswbc::Vision{std::vector<unswbc::Tile>{unswbc::Tile{{5+round,5}}}};
        fixture.game.round_num = round;
        world.update(fixture.controller,fixture.game);
    }
    CHECK(sudo_win::Simulation{}.initial_state(fixture.controller,&world).body
          == std::vector<unswbc::Position>{{8,5},{7,5},{6,5}});
}

TEST_CASE("delayed team reports preserve observation authority and bounded memory") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game.round_num = 10;
    auto world = sudo_win::WorldModel{fixture.game};
    fixture.tile({6,5}).dragon_part = unswbc::DragonPart{{6,5},9,unswbc::Team::B,unswbc::Direction::WEST,false};
    world.update(fixture.controller,fixture.game);
    world.receive_report({sudo_win::MessageType::empty,10,4097,6,5,0},10);
    CHECK(world.cell({6,5}).occupant.has_value());
    world.receive_report({sudo_win::MessageType::pearl,10,4097,0,0,1},10);
    CHECK_FALSE(world.has_seen({0,0}));
    CHECK_FALSE(world.cell({0,0}).has_pearl);
    auto const count = world.reports().size();
    world.receive_report({sudo_win::MessageType::champion,10,4097,5,5,10},10);
    world.receive_report({sudo_win::MessageType::pearl,11,4,5,5,1},10);
    world.receive_report({sudo_win::MessageType::pearl,1,4,5,5,1},10);
    world.receive_report({sudo_win::MessageType::pearl,10,4,63,63,1},10);
    CHECK(world.reports().size() == count);
    for (auto id = 2; id < 100; ++id) {
        world.receive_report({sudo_win::MessageType::pearl,10,id,5,5,1},10);
    }
    CHECK(world.reports().size() == 64);
    fixture.game.round_num = 19;
    world.update(fixture.controller,fixture.game);
    CHECK(world.reports().empty());
}

TEST_CASE("reported portal pairs provide static topology without certifying remote landings") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    fixture.tile({6,5}).get_edge(unswbc::Direction::WEST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    CHECK_FALSE(world.transition({5,5},unswbc::Direction::EAST));
    world.receive_report({sudo_win::MessageType::portal,1,4097,0,0,19},1);
    CHECK(world.transition({5,5},unswbc::Direction::EAST) == unswbc::Position{0,0});
    CHECK_FALSE(world.has_seen({0,0}));
    world.receive_report({sudo_win::MessageType::portal,1,4097,2,2,21},1);
    CHECK(world.portal_endpoints(10) == nullptr); // contradicts a directly seen empty edge
}

TEST_CASE("queen corridors preserve fixed identities and expire remote intentions") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 7;
    fixture.controller.unit_count = 2;
    fixture.tile({6,5}).dragon_part = unswbc::DragonPart{
        {6,5},1,unswbc::Team::A,unswbc::Direction::NORTH,true};
    for (auto const direction : {unswbc::Direction::NORTH, unswbc::Direction::EAST}) {
        fixture.tile({6,5}).get_edge(direction) = unswbc::Edge{false,unswbc::EdgeType::KELP};
    }
    fixture.tile({6,4}).dragon_part = unswbc::DragonPart{
        {6,4},1,unswbc::Team::A,unswbc::Direction::SOUTH,false};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto const at = [&](std::vector<int> const& weights, unswbc::Position p) {
        return weights[static_cast<std::size_t>(p.y * fixture.game.width + p.x)];
    };
    CHECK(at(world.queen_reservations(fixture.controller,0),{6,6}) > 0);
    fixture.tile({6,5}).dragon_part->team = unswbc::Team::B;
    world.update(fixture.controller,fixture.game);
    CHECK(at(world.queen_reservations(fixture.controller,0),{6,6}) == 0);
    world.receive_report({sudo_win::MessageType::danger,0,1,3,3,0},0);
    CHECK(at(world.queen_reservations(fixture.controller,1),{3,3}) > 0);
    CHECK(at(world.queen_reservations(fixture.controller,2),{3,3}) == 0);
    world.receive_report({sudo_win::MessageType::danger,0,7,4,4,0},0);
    CHECK(at(world.queen_reservations(fixture.controller,0),{4,4}) == 0);
}

TEST_CASE("portal surveys never advertise a tile our planned body will occupy") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    fixture.tile({6,5}).get_edge(unswbc::Direction::NORTH) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    fixture.tile({7,5}).pearl = true;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto action = sudo_win::PlannedAction{};
    action.steps = {unswbc::Direction::WEST};
    auto const survey = world.portal_survey(fixture.controller,0,action);
    REQUIRE(survey);
    CHECK(survey->x == 6);
    CHECK(survey->y == 5);
    CHECK(survey->value == 1033);
    action.steps = {unswbc::Direction::EAST};
    CHECK_FALSE(world.portal_survey(fixture.controller,0,action));
}

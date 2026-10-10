#include "sudo_win/safety/safety.h"
#include "sudo_win/combat/combat.h"
#include "sudo_win/world/world_model.h"
#include "sudo_win/planner/planner.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>
#include "sudo_win/planner/simulation.h"
#include <fstream>

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

TEST_CASE("remembered portal escapes keep stale observations separate from safe movement") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game.round_num = 10;
    for (auto const p : std::vector<unswbc::Position>{{5,6},{5,7}}) {
        fixture.tile(p).dragon_part = unswbc::DragonPart{p,0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    }
    for (auto y = 0; y <= 2; ++y) {
        for (auto x = 0; x <= 2; ++x) {
            if (fixture.controller.get_tile({x,y}) == nullptr) {
                fixture.controller.vision.tiles.emplace_back(unswbc::Position{x,y});
            }
        }
    }
    fixture.controller.vision = unswbc::Vision{fixture.controller.vision.tiles};
    fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    fixture.tile({0,0}).get_edge(unswbc::Direction::WEST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto const hide_exit = [&] {
        auto tiles = fixture.controller.vision.tiles;
        tiles.erase(std::remove_if(tiles.begin(),tiles.end(),[](auto const& tile) {
            auto p = tile.get_position();
            return p.x < 2 || p.y < 2;
        }),tiles.end());
        fixture.controller.vision = unswbc::Vision{std::move(tiles)};
        fixture.game.round_num = 12;
        world.update(fixture.controller,fixture.game);
    };
    auto const escape = [&] {
        return sudo_win::Safety{}.remembered_portal_escape(fixture.controller,world,fixture.game.get_round_num());
    };
    SECTION("a known empty remote exit offers an uncertain single step escape") {
        hide_exit();
        CHECK(escape() == unswbc::Direction::EAST);
        CHECK(sudo_win::Safety{}.standard_move_reason(fixture.controller,unswbc::Direction::EAST,&world)
              == sudo_win::SafetyReason::unknown_tile);
    }
    SECTION("expired empty observations cannot authorize a crossing") {
        hide_exit();
        fixture.game.round_num = 27;
        CHECK_FALSE(escape());
    }
    SECTION("remembered bodies block the exit") {
        fixture.tile({0,0}).dragon_part = unswbc::DragonPart{{0,0},4,unswbc::Team::B,unswbc::Direction::NORTH,false};
        world.update(fixture.controller,fixture.game);
        hide_exit();
        CHECK_FALSE(escape());
    }
    SECTION("a recent enemy head near the exit rules out the escape") {
        fixture.tile({1,1}).dragon_part = unswbc::DragonPart{{1,1},4,unswbc::Team::B,unswbc::Direction::NORTH,true};
        world.update(fixture.controller,fixture.game);
        hide_exit();
        CHECK_FALSE(escape());
    }
    SECTION("an exit with only one onward route is rejected") {
        fixture.tile({0,0}).get_edge(unswbc::Direction::SOUTH) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        world.update(fixture.controller,fixture.game);
        hide_exit();
        CHECK_FALSE(escape());
    }
    SECTION("incomplete body knowledge cannot rule out teleporting into ourselves") {
        hide_exit();
        fixture.tile({5,6}).dragon_part.reset();
        CHECK_FALSE(escape());
    }
    SECTION("the planner tries the portal when ordinary movement is trapped") {
        hide_exit();
        for (auto const d : {unswbc::Direction::NORTH,unswbc::Direction::WEST}) {
            fixture.tile({5,5}).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        }
        world.update(fixture.controller,fixture.game);
        auto const action = sudo_win::Planner{}.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::champion);
        CHECK(action.kind == sudo_win::ActionKind::move);
        REQUIRE(action.steps.size() == 1);
        CHECK(action.steps.front() == unswbc::Direction::EAST);
    }
    SECTION("a certified reversed tail rescue takes precedence over an uncertain portal") {
        fixture.controller.length = 4;
        fixture.tile({5,8}).dragon_part = unswbc::DragonPart{{5,8},0,unswbc::Team::A,unswbc::Direction::NORTH,false};
        world.update(fixture.controller,fixture.game);
        hide_exit();
        for (auto const d : {unswbc::Direction::NORTH,unswbc::Direction::WEST}) {
            fixture.tile({5,5}).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        }
        world.update(fixture.controller,fixture.game);
        auto const action = sudo_win::Planner{}.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::champion);
        CHECK(action.kind == sudo_win::ActionKind::split);
        CHECK(action.split_size == 2);
    }
    SECTION("a good visible escape takes precedence over an unseen portal exit") {
        hide_exit();
        auto const action = sudo_win::Planner{}.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::champion);
        REQUIRE(!action.steps.empty());
        CHECK(action.steps.front() != unswbc::Direction::EAST);
    }
}

TEST_CASE("helper portal exploration is bounded by role progress and cooldown") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    fixture.controller.unit_count = 2;
    fixture.controller.length = 3;
    for (auto& tile : fixture.controller.vision.tiles) {
        tile.pearl_time = -1;
    }
    for (auto const p : std::vector<unswbc::Position>{{5,6},{5,7}}) {
        fixture.tile(p).dragon_part = unswbc::DragonPart{p,4,unswbc::Team::A,unswbc::Direction::NORTH,false};
    }
    fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto planner = sudo_win::Planner{};
    auto const choose = [&] { return planner.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::scout); };
    SECTION("a stalled helper probes once then waits before trying again") {
        CHECK(choose().steps.front() != unswbc::Direction::EAST);
        fixture.game.round_num = 9;
        world.update(fixture.controller,fixture.game);
        auto const probe = choose();
        REQUIRE(probe.steps.size() == 1);
        CHECK(probe.steps.front() == unswbc::Direction::EAST);
        fixture.game.round_num = 10;
        CHECK(choose().steps.front() != unswbc::Direction::EAST);
    }
    SECTION("a designated scout starts exploring after three stalled rounds") {
        fixture.game.round_num = 0;
        choose();
        fixture.game.round_num = 3;
        world.update(fixture.controller,fixture.game);
        CHECK(choose().steps.front() == unswbc::Direction::EAST);
    }
    SECTION("fixed queens cannot be mistaken for expendable scouts") {
        fixture.controller.head.dragon_id = 0;
        CHECK_FALSE(sudo_win::Safety{}.helper_portal_probe(fixture.controller,world,9));
    }
    SECTION("the last survivor cannot probe unknown exits") {
        fixture.controller.unit_count = 1;
        CHECK_FALSE(sudo_win::Safety{}.helper_portal_probe(fixture.controller,world,9));
    }
    SECTION("aggregate enemy sonar contact defers blind exploration without locating the enemy") {
        fixture.controller.sonar_echoes.enemy_head = 1;
        CHECK_FALSE(sudo_win::Safety{}.helper_portal_probe(fixture.controller,world,9));
    }
    SECTION("visible food keeps the helper collecting locally") {
        choose();
        fixture.game.round_num = 9;
        fixture.tile({6,4}).pearl = true;
        world.update(fixture.controller,fixture.game);
        CHECK(choose().steps.front() != unswbc::Direction::EAST);
    }
}

TEST_CASE("portal destination surveys are advisory fresh and tied to a mapped pair") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.length = 3;
    fixture.controller.unit_count = 2;
    fixture.tile({5,6}).dragon_part = unswbc::DragonPart{{5,6},0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    fixture.tile({5,7}).dragon_part = unswbc::DragonPart{{5,7},0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    world.receive_report({sudo_win::MessageType::portal,0,4,0,0,19},0);
    world.receive_report({sudo_win::MessageType::empty,0,4,0,0,1033},0);
    CHECK(sudo_win::Safety{}.surveyed_portal_route(fixture.controller,world,1) == unswbc::Direction::EAST);
    CHECK_FALSE(world.has_seen({0,0}));
    CHECK(sudo_win::Safety{}.standard_move_reason(fixture.controller,unswbc::Direction::EAST,&world)
          == sudo_win::SafetyReason::unknown_tile);
    CHECK_FALSE(sudo_win::Safety{}.surveyed_portal_route(fixture.controller,world,2));
    fixture.controller.length = 9;
    CHECK_FALSE(sudo_win::Safety{}.surveyed_portal_route(fixture.controller,world,1));
    fixture.controller.length = 3;
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    fixture.game.round_num = 0;
    world.update(fixture.controller,fixture.game);
    auto planner = sudo_win::Planner{};
    static_cast<void>(planner.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::queen));
    fixture.game.round_num = 13;
    world.update(fixture.controller,fixture.game);
    world.receive_report({sudo_win::MessageType::empty,13,4,0,0,1033},13);
    auto const relocation = planner.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::queen);
    REQUIRE(relocation.steps.size() == 1);
    CHECK(relocation.steps.front() == unswbc::Direction::EAST);
}

TEST_CASE("a fresh productive portal survey cannot displace a viable queen farm") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.length = 3;
    fixture.controller.unit_count = 2;
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    fixture.tile({5,6}).dragon_part = unswbc::DragonPart{{5,6},0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    fixture.tile({5,7}).dragon_part = unswbc::DragonPart{{5,7},0,unswbc::Team::A,unswbc::Direction::NORTH,false};
    fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::PORTAL,9};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    world.receive_report({sudo_win::MessageType::portal,0,4,0,0,19},0);
    auto planner = sudo_win::Planner{false};
    static_cast<void>(planner.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::queen));
    fixture.game.round_num = 13;
    fixture.tile({5,2}).pearl = true;
    world.update(fixture.controller,fixture.game);
    world.receive_report({sudo_win::MessageType::empty,13,4,0,0,1033},13);
    REQUIRE(sudo_win::Safety{}.surveyed_portal_route(fixture.controller,world,13));
    auto const action = planner.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::queen);
    REQUIRE(action.steps.size() == 1);
    CHECK(action.steps.front() == unswbc::Direction::NORTH);
}

TEST_CASE("live portal neck traps retain unseen body occupancy across movement and splitting") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game = unswbc::Game{32,16,64};
    auto identity = 3;
    SECTION("helper three splits inside the trapped landing pocket") {}
    SECTION("helper twelve enters with food and keeps its unseen neck") { identity = 12; }
    fixture.controller.head.dragon_id = identity;
    auto input = std::ifstream{std::string{SUDO_WIN_TEST_SOURCE_DIR} + "/replays/portals_"
        + std::to_string(identity) + "_neck_trap.txt"};
    REQUIRE(input.good());
    auto world = sudo_win::WorldModel{fixture.game};
    for (auto turn = 0; turn < 3; ++turn) {
        auto* previous = std::cin.rdbuf(input.rdbuf());
        auto updated = false;
        try { updated = unswbc::update(fixture.controller,fixture.game); }
        catch (...) { std::cin.rdbuf(previous); throw; }
        std::cin.rdbuf(previous);
        REQUIRE(updated);
        world.update(fixture.controller,fixture.game);
        if (turn < 2) {
            auto action = sudo_win::PlannedAction{};
            if (identity == 3 && turn == 1) { action.kind = sudo_win::ActionKind::split; action.split_size = 2; }
            else { action.steps = {identity == 12 && turn == 0 ? unswbc::Direction::NORTH : unswbc::Direction::WEST}; }
            world.remember_action(fixture.controller,fixture.game,action);
        }
    }
    auto const exit = world.transition(fixture.controller.get_position(),unswbc::Direction::EAST);
    REQUIRE(exit);
    auto const state = sudo_win::Simulation{}.initial_state(fixture.controller,&world);
    REQUIRE(state.body.size() >= 2);
    CHECK(state.body[1] == *exit);
    CHECK(fixture.controller.get_tile(*exit) == nullptr);
    CHECK_FALSE(sudo_win::Simulation{}.advance(fixture.controller,state,unswbc::Direction::EAST,false,&world));
    CHECK_FALSE(sudo_win::Safety{}.helper_portal_probe(fixture.controller,world,fixture.game.get_round_num()));
    CHECK_FALSE(sudo_win::Safety{}.remembered_portal_escape(fixture.controller,world,fixture.game.get_round_num()));
}

TEST_CASE("a helper must not cover all queen exits with its resulting body") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    fixture.controller.length = 3;
    fixture.controller.unit_count = 2;
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    fixture.tile({5,4}).dragon_part = unswbc::DragonPart{{5,4},4,unswbc::Team::A,unswbc::Direction::SOUTH,false};
    fixture.tile({4,4}).dragon_part = unswbc::DragonPart{{4,4},4,unswbc::Team::A,unswbc::Direction::EAST,false};
    fixture.tile({6,4}).dragon_part = unswbc::DragonPart{{6,4},0,unswbc::Team::A,unswbc::Direction::NORTH,true};
    for (auto const d : {unswbc::Direction::NORTH,unswbc::Direction::EAST}) {
        fixture.tile({6,4}).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
    }
    fixture.tile({6,5}).pearl = true;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto const initial = sudo_win::Simulation{}.initial_state(fixture.controller,&world);
    auto const east = sudo_win::Simulation{}.advance(fixture.controller,initial,unswbc::Direction::EAST,false,&world);
    auto const west = sudo_win::Simulation{}.advance(fixture.controller,initial,unswbc::Direction::WEST,false,&world);
    REQUIRE(east);
    REQUIRE(west);
    CHECK(sudo_win::Safety{}.blocks_queen_escape(fixture.controller,*east,world));
    CHECK_FALSE(sudo_win::Safety{}.blocks_queen_escape(fixture.controller,*west,world));
    auto const action = sudo_win::Planner{false}.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::collector);
    REQUIRE(action.steps.size() == 1);
    CHECK(action.steps.front() != unswbc::Direction::EAST);
    // Another ally's obstruction is not something this helper can clear.
    fixture.tile({5,4}).dragon_part->dragon_id = 8;
    fixture.tile({6,5}).dragon_part = unswbc::DragonPart{{6,5},8,unswbc::Team::A,unswbc::Direction::NORTH,false};
    CHECK_FALSE(sudo_win::Safety{}.blocks_queen_escape(fixture.controller,*east,world));
}

TEST_CASE("validated paid movement can release jointly blocked queen exits") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    fixture.controller.head.position = {5,4};
    fixture.controller.length = 3;
    fixture.controller.unit_count = 2;
    for (auto& tile : fixture.controller.vision.tiles) { tile.pearl_time = -1; }
    fixture.tile({5,4}).dragon_part = unswbc::DragonPart{{5,4},4,unswbc::Team::A,unswbc::Direction::NORTH,true};
    fixture.tile({5,5}).dragon_part = unswbc::DragonPart{{5,5},4,unswbc::Team::A,unswbc::Direction::NORTH,false};
    fixture.tile({6,5}).dragon_part = unswbc::DragonPart{{6,5},4,unswbc::Team::A,unswbc::Direction::WEST,false};
    fixture.tile({6,4}).dragon_part = unswbc::DragonPart{{6,4},0,unswbc::Team::A,unswbc::Direction::NORTH,true};
    for (auto const d : {unswbc::Direction::NORTH,unswbc::Direction::EAST}) {
        fixture.tile({6,4}).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
    }
    fixture.tile({5,3}).pearl = true;
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto state = sudo_win::Simulation{}.initial_state(fixture.controller,&world);
    REQUIRE(sudo_win::Safety{}.blocks_queen_escape(fixture.controller,state,world));
    auto const action = sudo_win::Planner{}.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::collector);
    REQUIRE(action.steps.size() == 2);
    CHECK(action.reason == "paid queen corridor release");
    for (std::size_t i = 0; i < action.steps.size(); ++i) {
        auto const next = sudo_win::Simulation{}.advance(fixture.controller,state,action.steps[i],i > 0,&world);
        REQUIRE(next);
        state = *next;
    }
    CHECK(state.pearls == 1);
    CHECK(state.body.size() == 3);
    CHECK_FALSE(sudo_win::Safety{}.blocks_queen_escape(fixture.controller,state,world));
}

TEST_CASE("protected continuations distinguish legal exits from funded enemy pressure") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.tile({7,5}).dragon_part = unswbc::DragonPart{{7,5},7,unswbc::Team::B,unswbc::Direction::WEST,true};
    for (auto const d : {unswbc::Direction::NORTH,unswbc::Direction::WEST}) {
        fixture.tile({5,5}).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
    }
    auto after = sudo_win::SimulationState{};
    after.body = {{5,5},{5,6}};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto const combat = sudo_win::Combat{};
    CHECK(sudo_win::Safety{}.unpressured_exits(fixture.controller,after,world,
        combat.threats(fixture.controller,&world)) == 0);
    fixture.tile({5,5}).get_edge(unswbc::Direction::WEST) = unswbc::Edge{};
    world.update(fixture.controller,fixture.game);
    CHECK(sudo_win::Safety{}.unpressured_exits(fixture.controller,after,world,
        combat.threats(fixture.controller,&world)) == 1);
}

TEST_CASE("helpers preserve a continuation beyond the queen's sole immediate exit") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 4;
    fixture.controller.length = 2;
    fixture.controller.unit_count = 2;
    fixture.tile({6,3}).dragon_part = unswbc::DragonPart{
        {6,3},0,unswbc::Team::A,unswbc::Direction::SOUTH,true};
    for (auto const d : {unswbc::Direction::NORTH,unswbc::Direction::EAST,unswbc::Direction::WEST}) {
        fixture.tile({6,3}).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
    }
    for (auto const d : {unswbc::Direction::EAST,unswbc::Direction::WEST}) {
        fixture.tile({6,4}).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto after = sudo_win::SimulationState{};
    after.body = {{6,5},{5,5}};
    CHECK(sudo_win::Safety{}.blocks_queen_escape(fixture.controller,after,world));
    after.body = {{4,5},{5,5}};
    CHECK_FALSE(sudo_win::Safety{}.blocks_queen_escape(fixture.controller,after,world));
    // Do not attribute a pre-existing wall trap to this helper.
    fixture.tile({6,4}).get_edge(unswbc::Direction::SOUTH)
        = unswbc::Edge{false,unswbc::EdgeType::KELP};
    world.update(fixture.controller,fixture.game);
    after.body = {{6,5},{5,5}};
    CHECK_FALSE(sudo_win::Safety{}.blocks_queen_escape(fixture.controller,after,world));
}

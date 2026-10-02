#include "sudo_win/splitting/splitting.h"
#include "sudo_win/world/world_model.h"
#include "sudo_win/planner/planner.h"
#include "sudo_win/planner/simulation.h"
#include <algorithm>

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("well fed queens can invest while retaining a safe scoring parent") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto body = std::vector<unswbc::Position>{{5,5},{5,6},{4,6},{3,6},{2,6},{2,5},{2,4},{3,4}};
    fixture.controller.length = static_cast<int>(body.size());
    for (std::size_t i = 0; i < body.size(); ++i) {
        auto dir = unswbc::Direction{unswbc::Direction::NORTH};
        if (i > 0) {
            for (auto const d : unswbc::Direction::get_direction_list()) {
                if (body[i].add_dir(d) == body[i-1]) { dir = d; break; }
            }
        }
        fixture.tile(body[i]).dragon_part = unswbc::DragonPart{body[i],0,unswbc::Team::A,dir,i == 0};
    }
    fixture.tile({6,4}).pearl = true;
    fixture.tile({3,3}).pearl = true;
    auto const split = [&] {
        auto world = sudo_win::WorldModel{fixture.game};
        world.update(fixture.controller,fixture.game);
        return sudo_win::SplittingPolicy{}.grow_population(fixture.controller,fixture.game,sudo_win::Role::queen,world);
    };
    SECTION("early investment retains six queen segments and separate food") {
        auto const action = split();
        REQUIRE(action);
        CHECK(action->split_size == 2);
        REQUIRE(action->resource_target);
        CHECK(action->resource_target == unswbc::Position{6,4});
    }
    SECTION("a thin queen cannot fund investment") {
        fixture.controller.length = 7;
        CHECK_FALSE(split());
    }
    SECTION("established populations and late queens retain their scoring length") {
        fixture.controller.unit_count = 4;
        CHECK_FALSE(split());
        fixture.controller.unit_count = 1;
        fixture.game.round_num = 80;
        CHECK_FALSE(split());
    }
    SECTION("an enemy response prevents investment by a stationary queen") {
        fixture.tile({6,5}).dragon_part = unswbc::DragonPart{{6,5},7,unswbc::Team::B,unswbc::Direction::WEST,true};
        CHECK_FALSE(split());
    }
}

TEST_CASE("stable splitting policy") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.length = 20;
    auto policy = sudo_win::SplittingPolicy{};

    SECTION("splitting remains disabled") {
        CHECK_FALSE(policy.consider(fixture.controller, fixture.game, sudo_win::Role::collector, 49).has_value());
    }
}

TEST_CASE("experimental splits require safe parent child and separate pearl income") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 6;
    auto const body = std::vector<unswbc::Position>{
        {5, 5}, {5, 6}, {5, 7}, {4, 7}, {3, 7}, {2, 7}, {2, 6},
        {2, 5}, {2, 4}, {2, 3}, {3, 3}, {4, 3}, {5, 3}, {6, 3}};
    fixture.controller.length = static_cast<int>(body.size());
    for (std::size_t i = 0; i < body.size(); ++i) {
        auto heading = unswbc::Direction{unswbc::Direction::NORTH};
        if (i > 0) {
            for (auto const direction : unswbc::Direction::get_direction_list()) {
                if (body[i].add_dir(direction) == body[i - 1]) {
                    heading = direction;
                    break;
                }
            }
        }
        fixture.tile(body[i]).dragon_part = unswbc::DragonPart{
            body[i], 6, unswbc::Team::A, heading, i == 0};
    }
    for (auto& tile : fixture.controller.vision.tiles) {
        tile.pearl = tile.get_dragon() == nullptr;
        tile.pearl_time = -1;
    }
    // Investing is most useful when the parent has no immediate collection.
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        fixture.tile(fixture.controller.get_position().add_dir(direction)).pearl = false;
    }
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto policy = sudo_win::SplittingPolicy{};
    auto const candidate = [&] {
        return policy.consider(fixture.controller, fixture.game, sudo_win::Role::champion, 36, &world, true);
    };
    SECTION("a complete body with resources and escape routes supports a scored investment") {
        auto const split = candidate();
        REQUIRE(split);
        CHECK(split->kind == sudo_win::ActionKind::split);
        CHECK(fixture.controller.can_split(split->split_size));
        CHECK(split->score > 0);
        auto const action = sudo_win::Planner{false, true}.choose_action(fixture.controller, fixture.game,
                                                                         world, sudo_win::Role::champion);
        CHECK(action.kind == sudo_win::ActionKind::split);
    }
    SECTION("scarce resources prevent splitting") {
        for (auto& tile : fixture.controller.vision.tiles) {
            tile.pearl = false;
        }
        CHECK_FALSE(candidate());
    }
    SECTION("the team cap and late phase prevent splitting") {
        fixture.controller.unit_count = sudo_win::config::soft_unit_cap;
        CHECK_FALSE(candidate());
        fixture.controller.unit_count = 1;
        fixture.game.round_num = sudo_win::config::split_stop_round;
        CHECK_FALSE(candidate());
    }
    SECTION("incomplete body knowledge cannot certify the child") {
        fixture.tile({5, 6}).dragon_part.reset();
        CHECK_FALSE(candidate());
    }
    SECTION("a trapped tail cannot become a child") {
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            fixture.tile(body.back()).get_edge(direction)
                = unswbc::Edge{false, unswbc::EdgeType::KELP};
        }
        CHECK_FALSE(candidate());
    }
}

TEST_CASE("trapped snakes reverse their tails rather than collide") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.length = 6;
    auto const body = std::vector<unswbc::Position>{{5,5},{5,6},{4,6},{4,5},{4,4},{5,4}};
    for (std::size_t i = 0; i < body.size(); ++i) {
        auto heading = unswbc::Direction{unswbc::Direction::NORTH};
        if (i > 0) {
            for (auto const dir : unswbc::Direction::get_direction_list()) {
                if (body[i].add_dir(dir) == body[i-1]) { heading = dir; break; }
            }
        }
        fixture.tile(body[i]).dragon_part = unswbc::DragonPart{body[i],0,unswbc::Team::A,heading,i==0};
    }
    fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::KELP};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    SECTION("the visible tail has a certified escape") {
        auto const action = sudo_win::Planner{}.choose_action(fixture.controller,fixture.game,world,sudo_win::Role::champion);
        REQUIRE(action.kind == sudo_win::ActionKind::split);
        CHECK(fixture.controller.can_split(action.split_size));
        CHECK(action.split_size == 4);
        // The original head is trapped; its reversed tail can retain four of
        // the six segments and move north without crossing either new body.
        auto child = sudo_win::SimulationState{};
        child.body.assign(body.rbegin(), body.rbegin() + action.split_size);
        child.unranked_body.assign(body.begin(), body.end() - action.split_size);
        CHECK(sudo_win::Simulation{}.advance(fixture.controller, child,
                                             unswbc::Direction::NORTH, false, &world));
    }
    SECTION("a full team never emits an invalid rescue split") {
        fixture.controller.unit_count = fixture.controller.unit_limit;
        CHECK_FALSE(sudo_win::SplittingPolicy{}.rescue(fixture.controller,world,true));
    }
    SECTION("unknown tails are only split when movement is already fatal") {
        fixture.tile({4,6}).dragon_part.reset();
        CHECK_FALSE(sudo_win::SplittingPolicy{}.rescue(fixture.controller,world,false));
        REQUIRE(sudo_win::SplittingPolicy{}.rescue(fixture.controller,world,true));
    }
    SECTION("short snakes cannot pay for two legal bodies") {
        fixture.controller.length = 3;
        CHECK_FALSE(sudo_win::SplittingPolicy{}.rescue(fixture.controller,world,true));
    }
    SECTION("a child escape cannot certify rescue of a stationary trapped queen") {
        CHECK_FALSE(sudo_win::SplittingPolicy{}.rescue(fixture.controller,world,false,true));
        auto const fatal_fallback = sudo_win::SplittingPolicy{}.rescue(fixture.controller,world,true,true);
        REQUIRE(fatal_fallback);
        CHECK(fatal_fallback->score == 0);
        CHECK(fatal_fallback->split_size == 4);
    }
}

TEST_CASE("early expansion protects the champion and requires separate resources") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 6;
    auto const body = std::vector<unswbc::Position>{{5,5},{5,6},{4,6},{3,6},{2,6},{2,5}};
    fixture.controller.length = static_cast<int>(body.size());
    for (std::size_t i = 0; i < body.size(); ++i) {
        auto heading = unswbc::Direction{unswbc::Direction::NORTH};
        if (i > 0) {
            for (auto const direction : unswbc::Direction::get_direction_list()) {
                if (body[i].add_dir(direction) == body[i - 1]) {
                    heading = direction;
                    break;
                }
            }
        }
        fixture.tile(body[i]).dragon_part = unswbc::DragonPart{body[i], 6, unswbc::Team::A, heading, i == 0};
    }
    fixture.tile({6,4}).pearl = true;
    fixture.tile({2,4}).pearl = true;
    fixture.tile({3,4}).pearl = true;
    auto const candidate = [&](sudo_win::Role role = sudo_win::Role::collector) {
        auto world = sudo_win::WorldModel{fixture.game};
        world.update(fixture.controller, fixture.game);
        return sudo_win::SplittingPolicy{}.grow_population(fixture.controller, fixture.game, role, world);
    };
    SECTION("near-term independent spawners can fund territorial expansion") {
        fixture.tile({6,4}).pearl = false;
        fixture.tile({6,4}).pearl_time = 8;
        fixture.tile({2,4}).pearl = false;
        fixture.tile({2,4}).pearl_time = 10;
        fixture.tile({3,4}).pearl = false;
        CHECK(candidate());
        fixture.tile({2,4}).pearl_time = 30;
        CHECK_FALSE(candidate());
    }
    SECTION("the map budget allows teams that already start above eight units") {
        fixture.game.width = 20;
        fixture.game.height = 100;
        fixture.controller.unit_count = 9;
        CHECK(candidate());
        fixture.controller.unit_count = 21;
        CHECK_FALSE(candidate());
    }
    SECTION("rich early regions can fund population beyond the geometric budget") {
        fixture.game.width = 20;
        fixture.game.height = 100;
        fixture.controller.unit_count = 21;
        fixture.tile({7,4}).pearl = true;
        CHECK(candidate());
        fixture.controller.unit_limit = 21;
        CHECK_FALSE(candidate());
        fixture.controller.unit_limit = 64;
        fixture.game.round_num = 140;
        CHECK_FALSE(candidate());
    }
    SECTION("a legal two-segment child has income and two escape routes") {
        auto const split = candidate();
        REQUIRE(split);
        CHECK(split->split_size == 2);
        CHECK(fixture.controller.can_split(split->split_size));
        auto world = sudo_win::WorldModel{fixture.game};
        world.update(fixture.controller, fixture.game);
        auto const action = sudo_win::Planner{false, false, true}.choose_action(
            fixture.controller, fixture.game, world, sudo_win::Role::collector);
        CHECK(action.kind == sudo_win::ActionKind::split);
    }
    SECTION("productive narrow exits do not require two immediate branches") {
        fixture.tile({5,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        fixture.tile({5,5}).get_edge(unswbc::Direction::WEST) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        fixture.tile({2,5}).get_edge(unswbc::Direction::EAST) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        auto const split = candidate();
        REQUIRE(split);
        REQUIRE(split->resource_target);
        CHECK(split->resource_distance > 0);
    }
    SECTION("existing champions keep their length") {
        fixture.controller.unit_count = 2;
        CHECK_FALSE(candidate(sudo_win::Role::champion));
    }
    SECTION("a rich uncrowded region can invest its early champion after the initial population") {
        fixture.controller.unit_count = 4;
        fixture.game.round_num = 100;
        fixture.tile({7,4}).pearl = true;
        CHECK(candidate(sudo_win::Role::champion));
        fixture.game.round_num = 140;
        CHECK_FALSE(candidate(sudo_win::Role::champion));
    }
    SECTION("regional competitors stop investment despite a large global population budget") {
        fixture.controller.unit_count = 4;
        fixture.tile({7,4}).pearl = true;
        fixture.tile({7,7}).dragon_part = unswbc::DragonPart{{7,7},8,unswbc::Team::A,unswbc::Direction::NORTH,true};
        CHECK_FALSE(candidate(sudo_win::Role::champion));
    }
    SECTION("fixed queens never split for population investment") {
        CHECK_FALSE(candidate(sudo_win::Role::queen));
        fixture.controller.head.dragon_id = 1;
        CHECK_FALSE(candidate(sudo_win::Role::collector));
    }
    SECTION("a solitary early champion can start a second collector") {
        CHECK(candidate(sudo_win::Role::champion));
    }
    SECTION("scarcity and overcrowding prevent expansion") {
        fixture.tile({2,4}).pearl = false;
        fixture.tile({3,4}).pearl = false;
        CHECK_FALSE(candidate());
        fixture.tile({2,4}).pearl = true;
        fixture.controller.unit_count = sudo_win::config::population_unit_cap;
        CHECK_FALSE(candidate());
    }
    SECTION("unit limits and late rounds prevent invalid or late expansion") {
        fixture.controller.unit_count = fixture.controller.unit_limit;
        CHECK_FALSE(candidate());
        fixture.controller.unit_count = 1;
        fixture.game.round_num = 280;
        CHECK_FALSE(candidate());
    }
    SECTION("enemy attacks on a stationary parent prevent splitting") {
        fixture.tile({6,5}).dragon_part = unswbc::DragonPart{
            {6,5}, 3, unswbc::Team::B, unswbc::Direction::WEST, true};
        CHECK_FALSE(candidate());
    }
    SECTION("enemy attacks on the reversed tail prevent splitting") {
        fixture.tile({2,3}).dragon_part = unswbc::DragonPart{
            {2,3}, 3, unswbc::Team::B, unswbc::Direction::SOUTH, true};
        CHECK_FALSE(candidate());
    }
    SECTION("incomplete bodies cannot certify expansion") {
        fixture.tile({4,6}).dragon_part.reset();
        CHECK_FALSE(candidate());
    }
    SECTION("four segments can form two legal minimal collectors") {
        for (auto& tile : fixture.controller.vision.tiles) {
            tile.dragon_part.reset();
        }
        fixture.controller.length = 4;
        auto const short_body = std::vector<unswbc::Position>{{5,5},{5,6},{4,6},{4,5}};
        auto const headings = std::vector<unswbc::Direction>{unswbc::Direction::NORTH,
            unswbc::Direction::NORTH, unswbc::Direction::EAST, unswbc::Direction::SOUTH};
        for (std::size_t i = 0; i < short_body.size(); ++i) {
            fixture.tile(short_body[i]).dragon_part = unswbc::DragonPart{
                short_body[i], 6, unswbc::Team::A, headings[i], i == 0};
        }
        auto const split = candidate();
        REQUIRE(split);
        CHECK(fixture.controller.can_split(split->split_size));
    }
    SECTION("queen claims prevent funding expansion with her reserved income") {
        auto world = sudo_win::WorldModel{fixture.game};
        world.update(fixture.controller,fixture.game);
        world.receive_report({sudo_win::MessageType::feeder,1,0,6,4,1},1);
        CHECK_FALSE(sudo_win::SplittingPolicy{}.grow_population(fixture.controller,fixture.game,
            sudo_win::Role::collector,world));
    }
    SECTION("geometrically nearby food behind walls cannot fund the child") {
        for (auto const d : unswbc::Direction::get_direction_list()) {
            fixture.tile({2,4}).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
            fixture.tile({3,4}).get_edge(d) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        }
        // Block incoming sides too: fixture edges are independently editable.
        fixture.tile({2,5}).get_edge(unswbc::Direction::NORTH) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        fixture.tile({3,5}).get_edge(unswbc::Direction::NORTH) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        fixture.tile({2,3}).get_edge(unswbc::Direction::SOUTH) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        fixture.tile({3,3}).get_edge(unswbc::Direction::SOUTH) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        fixture.tile({4,4}).get_edge(unswbc::Direction::WEST) = unswbc::Edge{false,unswbc::EdgeType::KELP};
        CHECK_FALSE(candidate());
    }
}

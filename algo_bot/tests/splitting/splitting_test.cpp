#include "sudo_win/splitting/splitting.h"
#include "sudo_win/world/world_model.h"
#include "sudo_win/planner/planner.h"
#include <algorithm>

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

TEST_CASE("experimental splits require safe parent child and separate pearl income") {
    auto fixture = sudo_win::test::EngineFixture{};
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
            body[i], 0, unswbc::Team::A, heading, i == 0};
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

#include "sudo_win/economy/economy.h"
#include "sudo_win/bot/bot.h"
#include <sstream>

#include "sudo_win/config/config.h"
#include "sudo_win/pathfinding/pathfinding.h"
#include "sudo_win/world/world_model.h"

#include "../engine_fixture.h"

#include <catch2/catch.hpp>

TEST_CASE("economy destination scoring") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto const destination = unswbc::Position{6, 5};
    fixture.tile(destination).pearl = true;
    fixture.tile(destination).pearl_time = 3;

    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller, fixture.game);
    auto economy = sudo_win::Economy{};
    auto pathfinding = sudo_win::Pathfinding{};

    SECTION("immediate pearls receive dominant positive value") {
        auto const score = economy.score_destination(fixture.controller, world, pathfinding, destination);
        CHECK(score >= sudo_win::config::score_immediate_pearl);
    }
}

TEST_CASE("controlled feeding requires a ready fully observed queen and efficient safe pickup") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.game.round_num = 120;
    fixture.controller.head.dragon_id = 6;
    fixture.controller.length = 4;
    fixture.controller.unit_count = 3;
    auto const body = std::vector<unswbc::Position>{{5,5},{5,6},{4,6},{4,5}};
    auto const dirs = std::vector<unswbc::Direction>{unswbc::Direction::NORTH,unswbc::Direction::NORTH,
        unswbc::Direction::EAST,unswbc::Direction::SOUTH};
    for (std::size_t i = 0; i < body.size(); ++i) {
        fixture.tile(body[i]).dragon_part = unswbc::DragonPart{body[i],6,unswbc::Team::A,dirs[i],i == 0};
    }
    fixture.tile({6,5}).dragon_part = unswbc::DragonPart{{6,5},0,unswbc::Team::A,unswbc::Direction::SOUTH,true};
    fixture.tile({6,4}).dragon_part = unswbc::DragonPart{{6,4},0,unswbc::Team::A,unswbc::Direction::SOUTH,false};
    auto const donation = [&](sudo_win::Role role = sudo_win::Role::collector, int report_age = 0, int known_length = 2) {
        auto world = sudo_win::WorldModel{fixture.game};
        world.update(fixture.controller,fixture.game);
        world.receive_report({sudo_win::MessageType::champion,120 - report_age,0,6,5,known_length},120);
        return sudo_win::Economy{}.queen_donation(fixture.controller,fixture.game,world,role);
    };
    SECTION("half the donor body can reach queen score within three ordinary moves") {
        auto const action = donation();
        REQUIRE(action);
        CHECK(action->kind == sudo_win::ActionKind::donate);
        CHECK(action->recipient_id == 0);
        CHECK(action->resource_distance == 2);
        CHECK(action->resource_target == unswbc::Position{5,5});
        auto output = std::ostringstream{};
        auto* previous = std::cout.rdbuf(output.rdbuf());
        sudo_win::Bot::emit_action(fixture.controller,*action);
        std::cout.rdbuf(previous);
        CHECK(output.str() == "INDICATOR SUDO_WIN_DONATION 0 5 5 2\n");
        CHECK(output.str().find("MOVE ") == std::string::npos);
        CHECK(output.str().find("SPLIT ") == std::string::npos);
    }
    SECTION("missing or stale queen length cannot certify body release and pickup") {
        CHECK_FALSE(donation(sudo_win::Role::collector,1));
        CHECK_FALSE(donation(sudo_win::Role::collector,0,3));
    }
    SECTION("queen scorers and the last worker never retire for feeding") {
        CHECK_FALSE(donation(sudo_win::Role::champion));
        CHECK_FALSE(donation(sudo_win::Role::queen));
        fixture.controller.unit_count = 2;
        CHECK_FALSE(donation());
    }
    SECTION("enemy theft and a ready natural meal veto conversion") {
        fixture.tile({7,7}).dragon_part = unswbc::DragonPart{{7,7},7,unswbc::Team::B,unswbc::Direction::WEST,true};
        CHECK_FALSE(donation());
        fixture.tile({7,7}).dragon_part.reset();
        fixture.tile({7,5}).pearl = true;
        CHECK_FALSE(donation());
    }
    SECTION("incomplete donor bodies and blocked pickup paths cannot retire") {
        fixture.tile({5,6}).dragon_part.reset();
        CHECK_FALSE(donation());
    }
}

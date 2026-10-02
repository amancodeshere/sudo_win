#include "sudo_win/sonar/sonar.h"
#include "sudo_win/bot/bot.h"
#include <sstream>

#include <catch2/catch.hpp>

TEST_CASE("sonar codec") {
    auto codec = sudo_win::SonarCodec{};
    auto const original = sudo_win::TeamMessage{sudo_win::MessageType::enemy_head, 127, 42, 63, 7, 1200};
    auto const payload = codec.encode(original);

    SECTION("round trip preserves every field") {
        auto const decoded = codec.decode(payload, 130);
        REQUIRE(decoded.has_value());
        CHECK(decoded == original);
    }

    SECTION("modified payload fails authentication") {
        CHECK_FALSE(codec.decode(payload ^ (1ULL << 5U), 130).has_value());
    }

    SECTION("expired payload is rejected") {
        CHECK_FALSE(codec.decode(payload, 140).has_value());
    }
}

TEST_CASE("sonar reports distinguish teammates from a mirrored opponent bot") {
    auto codec = sudo_win::SonarCodec{};
    auto const report = sudo_win::TeamMessage{sudo_win::MessageType::champion, 10, 1, 5, 5, 20};
    auto const a = codec.encode(report,'A');
    auto const b = codec.encode(report,'B');
    REQUIRE(a != b);
    CHECK(codec.decode(a,11,'A'));
    CHECK(codec.decode(b,11,'B'));
    CHECK_FALSE(codec.decode(a,11,'B'));
    CHECK_FALSE(codec.decode(b,11,'A'));
    CHECK_FALSE(codec.decode(a,11,'C'));
}

TEST_CASE("sonar wire format preserves large identities and rejects field truncation") {
    auto codec = sudo_win::SonarCodec{};
    auto message = sudo_win::TeamMessage{sudo_win::MessageType::portal, 511, 4097, 63, 63, 2047};
    auto const decoded = codec.decode(codec.encode(message), 514);
    REQUIRE(decoded);
    CHECK(decoded->sender_id == 4097);
    CHECK(decoded->round == 511);
    message.sender_id = 8192;
    CHECK_FALSE(codec.can_encode(message));
    CHECK_THROWS_AS(codec.encode(message), std::invalid_argument);
    message.sender_id = 0;
    message.value = 2048;
    CHECK_FALSE(codec.can_encode(message));
}

#include "sudo_win/world/world_model.h"
#include "../engine_fixture.h"
#include <algorithm>

TEST_CASE("four beam scheduling carries all enemy heads and preserves relay provenance") {
    auto fixture = sudo_win::test::EngineFixture{};
    auto world = sudo_win::WorldModel{fixture.game};
    world.update(fixture.controller,fixture.game);
    auto scheduler = sudo_win::SonarScheduler{};
    auto const primary = sudo_win::TeamMessage{sudo_win::MessageType::champion,12,0,5,5,4};
    SECTION("dangerous helper heads have two beams alongside the primary claim") {
        fixture.tile({7,5}).dragon_part = unswbc::DragonPart{{7,5},7,unswbc::Team::B,unswbc::Direction::WEST,true};
        auto const messages = scheduler.schedule(fixture.controller,world,12,primary);
        CHECK(std::count(messages.begin(),messages.end(),primary) == 2);
        CHECK(std::count_if(messages.begin(),messages.end(),[](auto const& m) {
            return m.type == sudo_win::MessageType::enemy_head && m.value == 1 && m.x == 7 && m.y == 5;
        }) == 2);
    }
    SECTION("a relay keeps the original timestamp and sender and is not looped") {
        auto const original = sudo_win::TeamMessage{sudo_win::MessageType::feeder,10,4,0,0,3};
        world.receive_report(original,12);
        auto const messages = scheduler.schedule(fixture.controller,world,12,primary);
        CHECK(std::count(messages.begin(),messages.end(),original) == 1);
        auto const next = scheduler.schedule(fixture.controller,world,13,primary);
        CHECK(std::count(next.begin(),next.end(),original) == 0);
        auto const codec = sudo_win::SonarCodec{};
        REQUIRE(codec.decode(codec.encode(original),13));
        CHECK(codec.decode(codec.encode(original),13)->round == 10);
        CHECK_FALSE(codec.decode(codec.encode(original),19));
    }
    SECTION("aggregate echoes only increase urgency for a located fresh report") {
        fixture.controller.sonar_echoes.enemy_head = 1;
        auto const original = sudo_win::TeamMessage{sudo_win::MessageType::enemy_head,11,4,0,0,2};
        world.receive_report(original,12);
        auto const messages = scheduler.schedule(fixture.controller,world,12,primary);
        CHECK(std::count(messages.begin(),messages.end(),original) == 2);
        CHECK_FALSE(world.has_seen({0,0}));
        CHECK_FALSE(world.transition({0,0},unswbc::Direction::EAST));
    }
    SECTION("local evidence and short danger age suppress disproven or stale relays") {
        auto const stale = sudo_win::TeamMessage{sudo_win::MessageType::enemy_head,9,4,0,0,2};
        auto const disproven = sudo_win::TeamMessage{sudo_win::MessageType::pearl,11,6,6,6,1};
        world.receive_report(stale,12);
        world.receive_report(disproven,12);
        auto const messages = scheduler.schedule(fixture.controller,world,12,primary);
        CHECK(std::count(messages.begin(),messages.end(),primary) == 4);
    }
}

TEST_CASE("sonar identity overflow keeps a valid movement reply") {
    auto fixture = sudo_win::test::EngineFixture{};
    fixture.controller.head.dragon_id = 8192;
    fixture.controller.length = 2;
    fixture.controller.unit_count = 4;
    fixture.controller.head.dir = unswbc::Direction::SOUTH;
    fixture.tile({5,5}).dragon_part = fixture.controller.head;
    fixture.tile({5,4}).dragon_part = unswbc::DragonPart{{5,4},8192,unswbc::Team::A,unswbc::Direction::SOUTH,false};
    auto output = std::ostringstream{};
    auto* previous = std::cout.rdbuf(output.rdbuf());
    sudo_win::Bot{fixture.game}.execute_turn(fixture.controller,fixture.game);
    std::cout.rdbuf(previous);
    CHECK(output.str().find("MOVE ") != std::string::npos);
    CHECK(output.str().find("MOVE N\n") == std::string::npos); // The observed neck is occupied.
    CHECK(output.str().find("SONAR ") == std::string::npos);
}

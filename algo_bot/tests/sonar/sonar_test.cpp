#include "sudo_win/sonar/sonar.h"

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

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

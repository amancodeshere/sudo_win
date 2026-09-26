#include "sudo_win/sonar/sonar.h"

#include "../test_support.h"

namespace sudo_win::test {

auto run_sonar_tests(TestSuite& suite) -> void {
    auto codec = SonarCodec{};
    auto const original = TeamMessage{MessageType::enemy_head, 127, 42, 63, 7, 1200};
    auto const payload = codec.encode(original);
    auto const decoded = codec.decode(payload, 130);

    suite.expect(decoded.has_value(), "valid sonar payload is accepted");
    suite.expect(decoded == original, "sonar round trip preserves every field");
    suite.expect(!codec.decode(payload ^ (1ULL << 5U), 130).has_value(),
                 "modified sonar payload fails authentication");
    suite.expect(!codec.decode(payload, 140).has_value(), "expired sonar payload is rejected");
}

} // namespace sudo_win::test

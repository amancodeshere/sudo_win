#ifndef SUDO_WIN_SONAR_SONAR_H
#define SUDO_WIN_SONAR_SONAR_H

#include <cstdint>
#include <optional>

namespace sudo_win {

enum class MessageType : std::uint8_t {
    heartbeat = 0,
    pearl = 1,
    portal = 2,
    enemy_head = 3,
    champion = 4,
    danger = 5,
    feeder = 6,
    empty = 7,
};

struct TeamMessage {
    MessageType type = MessageType::heartbeat;
    int round = 0;
    int sender_id = 0;
    int x = 0;
    int y = 0;
    int value = 0;

    auto operator==(TeamMessage const&) const -> bool = default;
};

class SonarCodec {
public:
    [[nodiscard]] auto can_encode(TeamMessage const& message) const -> bool;
    [[nodiscard]] auto encode(TeamMessage const& message, char team = 'A') const -> std::uint64_t;
    [[nodiscard]] auto decode(std::uint64_t payload, int current_round, char team = 'A') const -> std::optional<TeamMessage>;

private:
    [[nodiscard]] auto tag(std::uint64_t body, char team) const -> std::uint16_t;
};

} // namespace sudo_win

#endif // SUDO_WIN_SONAR_SONAR_H

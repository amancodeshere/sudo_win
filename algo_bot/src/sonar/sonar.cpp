#include "../../include/sudo_win/sonar/sonar.h"

#include "../../include/sudo_win/config/config.h"

namespace sudo_win {
namespace {

constexpr auto body_mask = (1ULL << 48U) - 1ULL;

} // namespace

auto SonarCodec::encode(TeamMessage const& message) const -> std::uint64_t {
    auto body = std::uint64_t{0};
    body |= static_cast<std::uint64_t>(message.x & 0x3F);
    body |= static_cast<std::uint64_t>(message.y & 0x3F) << 6U;
    body |= static_cast<std::uint64_t>(message.round & 0x1FF) << 12U;
    body |= static_cast<std::uint64_t>(message.type) << 21U;
    body |= static_cast<std::uint64_t>(message.sender_id & 0xFF) << 25U;
    body |= static_cast<std::uint64_t>(message.value & 0x7FFF) << 33U;
    return body | (static_cast<std::uint64_t>(tag(body)) << 48U);
}

auto SonarCodec::decode(std::uint64_t payload, int current_round) const -> std::optional<TeamMessage> {
    auto const body = payload & body_mask;
    auto const supplied_tag = static_cast<std::uint16_t>(payload >> 48U);
    if (supplied_tag != tag(body)) {
        return std::nullopt;
    }

    auto message = TeamMessage{};
    message.x = static_cast<int>(body & 0x3F);
    message.y = static_cast<int>((body >> 6U) & 0x3F);
    message.round = static_cast<int>((body >> 12U) & 0x1FF);
    message.type = static_cast<MessageType>((body >> 21U) & 0x0F);
    message.sender_id = static_cast<int>((body >> 25U) & 0xFF);
    message.value = static_cast<int>((body >> 33U) & 0x7FFF);

    auto const current_mod = current_round & 0x1FF;
    auto const age = (current_mod - message.round + 512) % 512;
    if (age > config::sonar_max_age) {
        return std::nullopt;
    }
    return message;
}

auto SonarCodec::tag(std::uint64_t body) const -> std::uint16_t {
    auto mixed = body ^ config::sonar_secret;
    mixed ^= mixed >> 30U;
    mixed *= 0xBF58476D1CE4E5B9ULL;
    mixed ^= mixed >> 27U;
    mixed *= 0x94D049BB133111EBULL;
    mixed ^= mixed >> 31U;
    return static_cast<std::uint16_t>(mixed);
}

} // namespace sudo_win

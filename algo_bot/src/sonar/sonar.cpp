#include "../../include/sudo_win/sonar/sonar.h"

#include "../../include/sudo_win/config/config.h"
#include <stdexcept>
#include <algorithm>
#include "../../include/sudo_win/world/world_model.h"
#include "../../include/sudo_win/geometry/geometry.h"

namespace sudo_win {
namespace {

constexpr auto body_mask = (1ULL << 48U) - 1ULL;

} // namespace

auto SonarCodec::can_encode(TeamMessage const& message) const -> bool {
    return message.x >= 0 && message.x < 64 && message.y >= 0 && message.y < 64
        && message.round >= 0 && message.round < 512 && message.sender_id >= 0
        && message.sender_id < 8192 && message.value >= 0 && message.value < 2048
        && static_cast<unsigned>(message.type) <= 7;
}

auto SonarCodec::encode(TeamMessage const& message, char team) const -> std::uint64_t {
    if (!can_encode(message) || (team != 'A' && team != 'B')) {
        throw std::invalid_argument{"sonar fields exceed wire bounds"};
    }
    auto body = std::uint64_t{0};
    body |= static_cast<std::uint64_t>(message.x & 0x3F);
    body |= static_cast<std::uint64_t>(message.y & 0x3F) << 6U;
    body |= static_cast<std::uint64_t>(message.round & 0x1FF) << 12U;
    body |= static_cast<std::uint64_t>(message.type) << 21U;
    body |= static_cast<std::uint64_t>(message.sender_id) << 24U;
    body |= static_cast<std::uint64_t>(message.value) << 37U;
    return body | (static_cast<std::uint64_t>(tag(body, team)) << 48U);
}

auto SonarCodec::decode(std::uint64_t payload, int current_round, char team) const -> std::optional<TeamMessage> {
    auto const body = payload & body_mask;
    auto const supplied_tag = static_cast<std::uint16_t>(payload >> 48U);
    if ((team != 'A' && team != 'B') || supplied_tag != tag(body, team)) {
        return std::nullopt;
    }

    auto message = TeamMessage{};
    message.x = static_cast<int>(body & 0x3F);
    message.y = static_cast<int>((body >> 6U) & 0x3F);
    message.round = static_cast<int>((body >> 12U) & 0x1FF);
    message.type = static_cast<MessageType>((body >> 21U) & 0x07);
    message.sender_id = static_cast<int>((body >> 24U) & 0x1FFF);
    message.value = static_cast<int>((body >> 37U) & 0x7FF);

    auto const current_mod = current_round & 0x1FF;
    auto const age = (current_mod - message.round + 512) % 512;
    if (age > config::sonar_max_age) {
        return std::nullopt;
    }
    message.round = current_round - age;
    return message;
}

auto SonarScheduler::schedule(unswbc::Controller const& controller, WorldModel const& world,
                               int round, TeamMessage const& primary) -> std::array<TeamMessage, 4> {
    auto messages = std::array<TeamMessage, 4>{primary,primary,primary,primary};
    auto danger = std::optional<TeamMessage>{};
    auto nearest = 10000;
    for (auto const& tile : controller.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (part == nullptr || !part->is_head() || part->get_team() == controller.get_team()) { continue; }
        auto const distance = geometry::toroidal_manhattan(controller.get_position(), tile.get_position(),
                                                          world.width(), world.height());
        if (distance >= nearest) { continue; }
        nearest = distance;
        auto observed = 0;
        for (auto const& segment : controller.get_tiles()) {
            observed += segment.dragon_part && segment.dragon_part->get_id() == part->get_id();
        }
        danger = TeamMessage{MessageType::enemy_head,round,controller.get_id(),
            tile.get_position().x,tile.get_position().y,part->get_id() <= 1 ? 1024 : std::min(1023,observed)};
    }
    std::erase_if(relayed_, [&](auto const& message) { return round - message.round > config::sonar_max_age; });
    auto relay = std::optional<TeamMessage>{};
    auto priority = -1;
    for (auto const& report : world.reports()) {
        auto const age = round - report.round;
        if (report.sender_id == controller.get_id() || age < 0 || age > config::sonar_max_age
            || std::any_of(relayed_.begin(), relayed_.end(), [&](auto const& sent) {
                return sent.sender_id == report.sender_id && sent.type == report.type && sent.round >= report.round;
            })) { continue; }
        auto rank = -1;
        if (report.type == MessageType::enemy_head && age <= 2) { rank = 90; }
        if (report.type == MessageType::danger && age <= 1) { rank = 100; }
        if (report.type == MessageType::champion && age <= 2) { rank = 70; }
        if (report.type == MessageType::feeder && age <= 3) { rank = 60; }
        if (report.type == MessageType::empty && age <= 4) { rank = 40; }
        if (report.type == MessageType::portal) { rank = 30; }
        if (report.type == MessageType::pearl && age <= 4) { rank = 20; }
        auto const* local = controller.get_tile({report.x,report.y});
        if (local != nullptr && ((report.type == MessageType::pearl && !local->has_pearl())
            || (report.type == MessageType::enemy_head && (local->get_dragon() == nullptr
                || !local->get_dragon()->is_head() || local->get_dragon()->get_team() == controller.get_team())))) { continue; }
        if (rank - age > priority) { relay = report; priority = rank - age; }
    }
    if (relay) {
        // Preserve observation time and original identity; one relay per fresh
        // observation per unit bounds loops and cannot refresh a stale claim.
        if (relayed_.size() >= 64U) { relayed_.erase(relayed_.begin()); }
        relayed_.push_back(*relay);
        messages[2] = *relay;
    }
    if (danger) { messages[1] = *danger; messages[3] = *danger; }
    else if (relay && controller.get_sonar_echoes().enemy_head > 0 && relay->type == MessageType::enemy_head) {
        // Aggregate previous-turn echoes only change message urgency. They
        // contain no direction or coordinate and never certify free space.
        messages[1] = *relay;
    }
    // Cycle payload-to-direction assignment to spread information through
    // changing body/tail beam origins and corridors, without a fixed blind axis.
    std::rotate(messages.begin(),messages.begin() + ((round + controller.get_id()) % 4),messages.end());
    return messages;
}

auto SonarCodec::tag(std::uint64_t body, char team) const -> std::uint16_t {
    auto mixed = body ^ config::sonar_secret ^ (team == 'B' ? 0x9E3779B97F4A7C15ULL : 0ULL);
    mixed ^= mixed >> 30U;
    mixed *= 0xBF58476D1CE4E5B9ULL;
    mixed ^= mixed >> 27U;
    mixed *= 0x94D049BB133111EBULL;
    mixed ^= mixed >> 31U;
    return static_cast<std::uint16_t>((mixed & 0x7FFFULL) | (team == 'B' ? 0x8000ULL : 0ULL));
}

} // namespace sudo_win

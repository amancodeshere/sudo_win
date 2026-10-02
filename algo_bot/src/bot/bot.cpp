#include "../../include/sudo_win/bot/bot.h"

#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/planner/simulation.h"

#include <string>
#include <exception>
#include <algorithm>

namespace sudo_win {

Bot::Bot(unswbc::Game const& game)
: world_{game} {}

auto Bot::execute_turn(unswbc::Controller& controller, unswbc::Game const& game) -> void {
    auto action = PlannedAction{};
    auto report_payload = std::optional<std::uint64_t>{};
    auto beam_payloads = std::optional<std::array<std::uint64_t,4>>{};
#ifndef SUDO_WIN_DEVELOPMENT
    try {
#endif
    world_.update(controller, game);

    for (auto const payload : controller.get_sonar_messages()) {
        if (!config::enable_sonar) {
            break;
        }
        auto const message = sonar_.decode(payload, game.get_round_num(), static_cast<char>(controller.get_team().value));
        if (!message.has_value()) {
            continue;
        }
        world_.receive_report(*message, game.get_round_num());
    }

    auto const role = roles_.choose_role(controller, game, &world_);
    action = planner_.choose_action(controller, game, world_, role);
    if (config::enable_sonar) {
        auto report = std::optional<TeamMessage>{};
        auto const phase = game.get_round_num() % 4;
        auto const own = controller.get_position();
        if (action.kind == ActionKind::split && action.resource_target) {
            report = TeamMessage{MessageType::feeder,game.get_round_num() & 511,controller.get_id(),
                action.resource_target->x,action.resource_target->y,std::min(2047,action.resource_distance + 1)};
        } else if (controller.get_id() <= 1 && phase % 2 == 1) {
            report = world_.queen_intent(controller, game.get_round_num(), action);
        } else if (phase == 0 && (controller.get_id() <= 1
            || (config::enable_champion_farms && role == Role::champion))) {
            auto length = controller.get_length();
            auto position = own;
            if (config::enable_champion_farms && action.kind == ActionKind::split) {
                length -= action.split_size;
            } else if (config::enable_champion_farms) {
                auto state = Simulation{}.initial_state(controller, &world_);
                for (std::size_t i = 0; i < action.steps.size(); ++i) {
                    auto const next = Simulation{}.advance(controller, state, action.steps[i], i > 0, &world_);
                    if (!next) { break; }
                    state = *next;
                    length = static_cast<int>(state.body.size());
                    position = state.body.front();
                }
            }
            report = TeamMessage{controller.get_id() <= 1 ? MessageType::champion : MessageType::heartbeat, game.get_round_num() & 511,
                controller.get_id(), position.x, position.y, std::min(length, 2047)};
        } else if (phase == 1 || (config::enable_champion_farms && phase == 2 && controller.get_id() <= 1)) {
            if (auto const route = Pathfinding{}.remembered_target(controller, world_, game.get_round_num(), std::nullopt,
                    role == Role::queen || role == Role::champion);
                route && route->pearl) {
                report = TeamMessage{MessageType::feeder, game.get_round_num() & 511,
                    controller.get_id(), route->target.x, route->target.y, std::min(route->distance, 2047)};
            }
        } else if (phase == 2) {
            auto portals = std::vector<TeamMessage>{};
            for (auto const& tile : controller.get_tiles()) {
                for (auto const direction : {unswbc::Direction::NORTH, unswbc::Direction::WEST}) {
                    auto const& edge = tile.get_edge(direction);
                    auto const p = tile.get_position();
                    if (edge.is_portal() && edge.get_portal_id() >= 0 && edge.get_portal_id() < 1024) {
                        portals.push_back({MessageType::portal, game.get_round_num() & 511,
                            controller.get_id(), p.x, p.y, edge.get_portal_id() * 2
                                + (direction == unswbc::Direction::WEST ? 1 : 0)});
                    }
                }
            }
            if (!portals.empty()) {
                report = portals[static_cast<std::size_t>(game.get_round_num() / 4) % portals.size()];
            }
        } else {
            for (auto const& tile : controller.get_tiles()) {
                auto const p = tile.get_position();
                auto const* part = tile.get_dragon();
                if (part != nullptr && part->is_head() && part->get_id() <= 1
                    && part->get_team() != controller.get_team()) {
                    report = TeamMessage{MessageType::enemy_head, game.get_round_num() & 511,
                        controller.get_id(), p.x, p.y, 1024};
                    break;
                }
                if (!report && tile.has_pearl()) {
                    report = TeamMessage{MessageType::pearl, game.get_round_num() & 511,
                        controller.get_id(), p.x, p.y, 1};
                }
            }
            if (config::enable_portal_routing && controller.get_id() > 1
                && (!report || report->type != MessageType::enemy_head)) {
                if (auto const survey = world_.portal_survey(controller, game.get_round_num(), action)) {
                    report = survey;
                }
            }
        }
        if (config::enable_sonar_network) {
            auto state = Simulation{}.initial_state(controller, &world_);
            if (action.kind == ActionKind::split) {
                state.body.resize(state.body.size() - static_cast<std::size_t>(action.split_size));
            } else {
                for (std::size_t i = 0; i < action.steps.size(); ++i) {
                    auto next = Simulation{}.advance(controller,state,action.steps[i],i > 0,&world_);
                    if (!next) { break; }
                    state = std::move(*next);
                }
            }
            auto const fallback = TeamMessage{controller.get_id() <= 1 ? MessageType::champion : MessageType::heartbeat,
                game.get_round_num(),controller.get_id(),state.body.front().x,state.body.front().y,
                std::min(2047,static_cast<int>(state.body.size()))};
            auto const messages = sonar_scheduler_.schedule(controller,world_,game.get_round_num(),report.value_or(fallback));
            auto payloads = std::array<std::uint64_t,4>{};
            for (std::size_t i = 0; i < messages.size(); ++i) {
                if (sonar_.can_encode(messages[i])) {
                    payloads[i] = sonar_.encode(messages[i],static_cast<char>(controller.get_team().value));
                }
            }
            beam_payloads = payloads;
        }
        if (config::enable_portal_hazards && beam_payloads) {
            if (auto const warning = world_.portal_warning(controller,game.get_round_num()); warning && sonar_.can_encode(*warning)) {
                (*beam_payloads)[static_cast<std::size_t>((game.get_round_num() + controller.get_id()) % 4)]
                    = sonar_.encode(*warning,static_cast<char>(controller.get_team().value));
            }
        }
        if (report && sonar_.can_encode(*report)) {
            report_payload = sonar_.encode(*report, static_cast<char>(controller.get_team().value));
        }
    }
    world_.remember_action(controller, game, action);
#ifndef SUDO_WIN_DEVELOPMENT
    } catch (std::exception const&) {
        // No action has been emitted yet. Keep the competition reply valid.
        controller.make_move(Safety{}.least_bad_fallback(controller));
        return;
    }
#endif

    if (config::enable_indicators) {
        controller.set_indicator_string(std::string{action.reason} + "; growth: " + std::string{planner_.growth_rejection()});
    }
    emit_action(controller, action);
    if (action.kind == ActionKind::donate) { return; }
    if (beam_payloads) {
        auto const directions = unswbc::Direction::get_direction_list();
        for (std::size_t i = 0; i < directions.size(); ++i) {
            if ((*beam_payloads)[i] != 0) { controller.send_sonar(directions[i], (*beam_payloads)[i]); }
        }
    } else if (report_payload) {
        // Directed 64-bit messages, after the action. Rotate opposite beams;
        // delayed aggregate echoes are never treated as empty-space evidence.
        auto const beam_phase = game.get_round_num() + (config::enable_portal_routing ? game.get_round_num() / 4 : 0);
        auto const direction = unswbc::Direction{beam_phase % 2 == 0
            ? unswbc::Direction::NORTH : unswbc::Direction::EAST};
        controller.send_sonar(direction, *report_payload);
        controller.send_sonar(direction.get_opposite(), *report_payload);
    }
}

auto Bot::emit_action(unswbc::Controller& controller, PlannedAction const& action) -> void {
    if (action.kind == ActionKind::donate) {
        // The engine's default action retires the unit. A recognized diagnostic
        // command records intent; do not invent an unsupported SUICIDE opcode.
        controller.set_indicator_string("SUDO_WIN_DONATION " + std::to_string(action.recipient_id) + " "
            + std::to_string(action.resource_target->x) + " " + std::to_string(action.resource_target->y)
            + " " + std::to_string(action.resource_distance));
        return;
    } else if (action.kind == ActionKind::split) {
        controller.do_split(action.split_size);
        return;
    }
    if (action.kind == ActionKind::sprint && action.steps.size() > 1) {
        controller.make_moves(action.steps);
        return;
    }
    if (!action.steps.empty()) {
        controller.make_move(action.steps.front());
        return;
    }
    controller.make_move(unswbc::Direction::NORTH);
}

} // namespace sudo_win

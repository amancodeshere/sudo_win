#include "../../include/sudo_win/economy/economy.h"

#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/pathfinding/pathfinding.h"
#include "../../include/sudo_win/world/world_model.h"

#include <algorithm>
#include <limits>
#include "../../include/sudo_win/planner/simulation.h"
#include "../../include/sudo_win/combat/combat.h"
#include "../../include/sudo_win/geometry/geometry.h"

namespace sudo_win {

auto Economy::queen_donation(unswbc::Controller const& controller, unswbc::Game const& game,
                              WorldModel const& world, Role role) const -> std::optional<PlannedAction> {
    if (controller.get_id() <= 1 || controller.get_unit_count() < 3 || controller.get_length() < 4
        || controller.get_length() > 12 || game.get_round_num() < 120 || role == Role::champion
        || role == Role::queen) { return std::nullopt; }
    auto const simulation = Simulation{};
    auto const donor = simulation.initial_state(controller,&world);
    if (!donor.unranked_body.empty() || std::any_of(donor.body.begin(),donor.body.end(),[&](auto p) {
        return p.x < 0 || p.y < 0 || controller.get_tile(p) == nullptr;
    })) { return std::nullopt; }
    auto queen = std::optional<unswbc::DragonPart>{};
    auto queen_parts = std::vector<unswbc::Position>{};
    for (auto const& tile : controller.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (part != nullptr && part->get_id() <= 1 && part->get_team() == controller.get_team()) {
            queen_parts.push_back(tile.get_position());
            if (part->is_head()) { queen = *part; }
        }
    }
    if (!queen || queen_parts.size() < 2U) { return std::nullopt; }
    auto complete_queen = false;
    for (auto const& report : world.reports()) {
        complete_queen = complete_queen || (report.type == MessageType::champion
            && report.sender_id == queen->get_id() && report.round == game.get_round_num()
            && report.x == queen->get_position().x && report.y == queen->get_position().y
            && report.value == static_cast<int>(queen_parts.size()));
    }
    if (!complete_queen) { return std::nullopt; }
    // No retirement where an observed/reported enemy can contest the transfer.
    auto const near = [&](unswbc::Position p) {
        return geometry::toroidal_manhattan(p,queen->get_position(),world.width(),world.height()) <= 5
            || std::any_of(donor.body.begin(),donor.body.end(),[&](auto d) {
                return geometry::toroidal_manhattan(p,d,world.width(),world.height()) <= 4;
            });
    };
    for (auto const& tile : controller.get_tiles()) {
        auto const* part = tile.get_dragon();
        if (part != nullptr && part->is_head() && part->get_team() != controller.get_team()
            && near(tile.get_position())) { return std::nullopt; }
    }
    for (auto const& report : world.reports()) {
        if (report.type == MessageType::enemy_head && game.get_round_num() - report.round <= 2
            && near({report.x,report.y})) { return std::nullopt; }
    }
    auto recipient = controller;
    recipient.head = *queen;
    recipient.length = static_cast<int>(queen_parts.size());
    auto dropped = std::vector<unswbc::Position>{};
    for (std::size_t i = 0; i < donor.body.size(); ++i) {
        auto* tile = recipient.get_tile(donor.body[i]);
        tile->dragon_part.reset();
        if (i % 2U == 0U && !tile->has_pearl()) { dropped.push_back(donor.body[i]); tile->pearl = true; }
    }
    // A ready natural meal has higher conversion efficiency than retiring a worker.
    for (auto const d : unswbc::Direction::get_direction_list()) {
        auto const p = world.transition(queen->get_position(),d);
        auto const* tile = p ? controller.get_tile(*p) : nullptr;
        if (tile && tile->has_pearl() && !tile->get_dragon()) { return std::nullopt; }
    }
    struct Node { SimulationState state; std::vector<unswbc::Position> path; };
    auto queue = std::vector<Node>{{simulation.initial_state(recipient,&world),{}}};
    auto budget = 64;
    auto best = std::optional<PlannedAction>{};
    auto best_pickup = 0;
    for (std::size_t cursor = 0; cursor < queue.size() && budget > 0; ++cursor) {
        auto const node = queue[cursor];
        if (node.path.size() >= 3U) { continue; }
        for (auto const d : unswbc::Direction::get_direction_list()) {
            if (budget-- <= 0) { break; }
            auto next = simulation.advance(recipient,node.state,d,false,&world);
            if (!next || std::find(queen_parts.begin(),queen_parts.end(),next->body.front()) != queen_parts.end()
                || std::find(node.path.begin(),node.path.end(),next->body.front()) != node.path.end()) { continue; }
            auto path = node.path;
            path.push_back(next->body.front());
            auto pickup = 0;
            for (auto const p : dropped) {
                pickup += std::find(next->eaten.begin(),next->eaten.end(),p) != next->eaten.end();
            }
            if (pickup >= 2 && pickup * 2 >= controller.get_length() && pickup > best_pickup) {
                auto survival_budget = 128;
                auto const response = Combat{}.response_threat(recipient,*next,world);
                if (simulation.survival_depth(recipient,*next,6,survival_budget,&world) == 6
                    && response.funded_steps == 0 && response.possible_steps == 0 && response.unresolved_steps == 0) {
                    auto first = std::find_if(path.begin(),path.end(),[&](auto p) {
                        return std::find(dropped.begin(),dropped.end(),p) != dropped.end();
                    });
                    best_pickup = pickup;
                    auto action = PlannedAction{};
                    action.kind = ActionKind::donate;
                    action.resource_target = *first;
                    action.resource_distance = pickup;
                    action.recipient_id = queen->get_id();
                    action.score = pickup * config::score_immediate_pearl;
                    action.reason = "starved worker converts safely accessible body food to queen score";
                    best = action;
                }
            }
            queue.push_back({std::move(*next),std::move(path)});
        }
    }
    return best;
}

auto Economy::score_destination(unswbc::Controller const& controller,
                                WorldModel const& world,
                                Pathfinding const& pathfinding,
                                unswbc::Position destination) const -> int {
    auto score = 0;
    auto const* tile = controller.get_tile(destination);
    if (tile == nullptr) {
        return score;
    }

    if (tile->has_pearl()) {
        score += config::score_immediate_pearl;
    }
    if (tile->get_pearl_time() >= 0) {
        score += std::max(0, 20 - tile->get_pearl_time()) * config::score_future_pearl_step;
    }

    auto const distance = pathfinding.visible_pearl_distance(controller, destination, &world);
    if (distance != std::numeric_limits<int>::max()) {
        score += std::max(0, config::score_nearby_pearl_base - distance * config::score_nearby_pearl_step);
    }
    return score;
}

} // namespace sudo_win

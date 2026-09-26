#include "../../include/sudo_win/economy/economy.h"

#include "../../include/sudo_win/config/config.h"
#include "../../include/sudo_win/pathfinding/pathfinding.h"
#include "../../include/sudo_win/world/world_model.h"

#include <algorithm>
#include <limits>

namespace sudo_win {

auto Economy::score_destination(unswbc::Controller const& controller,
                                WorldModel const& world,
                                Pathfinding const& pathfinding,
                                unswbc::Position destination) const -> int {
    auto score = world.unseen_neighbour_count(destination) * config::score_frontier;
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

    auto const distance = pathfinding.visible_pearl_distance(controller, destination);
    if (distance != std::numeric_limits<int>::max()) {
        score += std::max(0, config::score_nearby_pearl_base - distance * config::score_nearby_pearl_step);
    }
    return score;
}

} // namespace sudo_win

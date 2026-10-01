#ifndef SUDO_WIN_PLANNER_SIMULATION_H
#define SUDO_WIN_PLANNER_SIMULATION_H

#include "../engine/helper.h"

#include <optional>
#include <vector>

namespace sudo_win {
class WorldModel;

struct SimulationState {
    // Head first. Unseen segments have the sentinel {-1, -1}.
    std::vector<unswbc::Position> body;
    std::vector<unswbc::Position> unranked_body;
    std::vector<unswbc::Position> eaten;
    int pearls = 0;
};

class Simulation {
public:
    [[nodiscard]] auto initial_state(unswbc::Controller const& controller,
                                     WorldModel const* world = nullptr) const -> SimulationState;
    [[nodiscard]] auto advance(unswbc::Controller const& controller,
                               SimulationState const& state,
                               unswbc::Direction direction,
                               bool pay_sprint = false,
                               WorldModel const* world = nullptr) const -> std::optional<SimulationState>;
    [[nodiscard]] auto survival_depth(unswbc::Controller const& controller,
                                      SimulationState const& state,
                                      int remaining_depth,
                                      int& node_budget,
                                      WorldModel const* world = nullptr) const -> int;
};

} // namespace sudo_win

#endif // SUDO_WIN_PLANNER_SIMULATION_H

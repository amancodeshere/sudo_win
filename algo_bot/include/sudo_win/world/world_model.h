#ifndef SUDO_WIN_WORLD_WORLD_MODEL_H
#define SUDO_WIN_WORLD_WORLD_MODEL_H

#include "../engine/helper.h"

#include <array>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>

namespace sudo_win {

struct EdgeKnowledge {
    bool seen = false;
    unswbc::EdgeType type = unswbc::EdgeType::EMPTY;
    int portal_id = -1;
};

struct OccupantKnowledge {
    char team = 'A';
    int id = -1;
    char direction = 'N';
    bool is_head = false;
};

struct CellKnowledge {
    bool seen = false;
    int last_seen_round = -1;
    bool has_pearl = false;
    int pearl_time = -1;
    std::array<EdgeKnowledge, 4> edges{};
    std::optional<OccupantKnowledge> occupant;
};

struct PortalEndpoint {
    unswbc::Position position;
    unswbc::Direction direction;
};

class WorldModel {
public:
    explicit WorldModel(unswbc::Game const& game);

    auto update(unswbc::Controller const& controller, unswbc::Game const& game) -> void;

    [[nodiscard]] auto cell(unswbc::Position position) const -> CellKnowledge const&;
    [[nodiscard]] auto has_seen(unswbc::Position position) const -> bool;
    [[nodiscard]] auto unseen_neighbour_count(unswbc::Position position) const -> int;
    [[nodiscard]] auto portal_endpoints(int portal_id) const -> std::vector<PortalEndpoint> const*;
    [[nodiscard]] auto width() const -> int;
    [[nodiscard]] auto height() const -> int;

private:
    [[nodiscard]] auto index(unswbc::Position position) const -> std::size_t;
    auto remember_portal(int portal_id, PortalEndpoint endpoint) -> void;

    int width_;
    int height_;
    std::vector<CellKnowledge> cells_;
    std::unordered_map<int, std::vector<PortalEndpoint>> portals_;
};

} // namespace sudo_win

#endif // SUDO_WIN_WORLD_WORLD_MODEL_H

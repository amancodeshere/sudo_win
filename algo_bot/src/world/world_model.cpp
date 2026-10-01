#include "../../include/sudo_win/world/world_model.h"

#include "../../include/sudo_win/geometry/geometry.h"

#include <algorithm>
#include <cstddef>

namespace sudo_win {
namespace {
[[nodiscard]] auto canonical_endpoint(PortalEndpoint endpoint) -> PortalEndpoint {
    if (endpoint.direction == unswbc::Direction::SOUTH) {
        return {endpoint.position.add_dir(endpoint.direction), unswbc::Direction::NORTH};
    }
    if (endpoint.direction == unswbc::Direction::EAST) {
        return {endpoint.position.add_dir(endpoint.direction), unswbc::Direction::WEST};
    }
    return endpoint;
}
} // namespace

WorldModel::WorldModel(unswbc::Game const& game)
: width_{game.width}
, height_{game.height}
, cells_(static_cast<std::size_t>(game.width * game.height)) {}

auto WorldModel::update(unswbc::Controller const& controller, unswbc::Game const& game) -> void {
    cells_[index(controller.get_position())].last_visited_round = game.get_round_num();
    for (auto const& tile : controller.get_tiles()) {
        auto& remembered = cells_[index(tile.get_position())];
        remembered.seen = true;
        remembered.last_seen_round = game.get_round_num();
        remembered.has_pearl = tile.has_pearl();
        remembered.pearl_time = tile.get_pearl_time();
        remembered.occupant.reset();

        if (auto const* part = tile.get_dragon()) {
            remembered.occupant = OccupantKnowledge{
                static_cast<char>(part->get_team().value),
                part->get_id(),
                static_cast<char>(part->get_dir().value),
                part->is_head(),
            };
        }

        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const& edge = tile.get_edge(direction);
            auto& knowledge = remembered.edges[geometry::direction_index(direction)];
            knowledge.seen = true;
            knowledge.type = edge.get_edge_type();
            knowledge.portal_id = edge.get_portal_id();

            if (edge.is_portal()) {
                remember_portal(edge.get_portal_id(), PortalEndpoint{tile.get_position(), direction});
            }
        }
    }
}

auto WorldModel::cell(unswbc::Position position) const -> CellKnowledge const& {
    return cells_[index(position)];
}

auto WorldModel::has_seen(unswbc::Position position) const -> bool {
    return cell(position).seen;
}

auto WorldModel::unseen_neighbour_count(unswbc::Position position) const -> int {
    auto unseen = 0;
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        if (!has_seen(position.add_dir(direction))) {
            ++unseen;
        }
    }
    return unseen;
}

auto WorldModel::portal_endpoints(int portal_id) const -> std::vector<PortalEndpoint> const* {
    auto const found = portals_.find(portal_id);
    return found == portals_.end() ? nullptr : &found->second;
}

auto WorldModel::transition(unswbc::Position from, unswbc::Direction direction) const
    -> std::optional<unswbc::Position> {
    auto const& edge = cell(from).edges[geometry::direction_index(direction)];
    if (!edge.seen || edge.type == unswbc::EdgeType::KELP) {
        return std::nullopt;
    }
    if (edge.type == unswbc::EdgeType::EMPTY) {
        return from.add_dir(direction);
    }
    auto const* endpoints = portal_endpoints(edge.portal_id);
    if (endpoints == nullptr || endpoints->size() != 2
        || endpoints->front().direction != endpoints->back().direction) {
        return std::nullopt;
    }
    auto const source = canonical_endpoint({from, direction});
    for (std::size_t i = 0; i < endpoints->size(); ++i) {
        if ((*endpoints)[i].position == source.position && (*endpoints)[i].direction == source.direction) {
            auto const partner = (*endpoints)[1 - i].position;
            return direction == unswbc::Direction::NORTH || direction == unswbc::Direction::WEST
                 ? partner.add_dir(direction) : partner;
        }
    }
    return std::nullopt;
}

auto WorldModel::width() const -> int {
    return width_;
}

auto WorldModel::height() const -> int {
    return height_;
}

auto WorldModel::index(unswbc::Position position) const -> std::size_t {
    auto const x = geometry::wrap(position.x, width_);
    auto const y = geometry::wrap(position.y, height_);
    return static_cast<std::size_t>(y * width_ + x);
}

auto WorldModel::remember_portal(int portal_id, PortalEndpoint endpoint) -> void {
    endpoint = canonical_endpoint(endpoint);
    auto& endpoints = portals_[portal_id];
    auto const duplicate = std::ranges::any_of(endpoints, [&](PortalEndpoint const& existing) {
        return existing.position == endpoint.position && existing.direction == endpoint.direction;
    });
    if (!duplicate) {
        endpoints.push_back(endpoint);
    }
}

} // namespace sudo_win

#include "../../include/sudo_win/world/world_model.h"

#include "../../include/sudo_win/geometry/geometry.h"
#include "../../include/sudo_win/planner/simulation.h"
#include "../../include/sudo_win/config/config.h"

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
    std::erase_if(reports_, [&](auto const& report) {
        return game.get_round_num() - report.round > config::sonar_max_age;
    });
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
    reconcile_body(controller, game);
}

auto WorldModel::receive_report(TeamMessage const& message, int round) -> void {
    if (message.x < 0 || message.x >= width_ || message.y < 0 || message.y >= height_
        || round < message.round || round - message.round > config::sonar_max_age
        || message.sender_id < 0 || message.sender_id >= 8192 || message.value < 0 || message.value >= 2048
        || static_cast<unsigned>(message.type) > 7
        || ((message.type == MessageType::champion || message.type == MessageType::danger)
            && message.sender_id > 1)) {
        return;
    }
    auto const found = std::find_if(reports_.begin(), reports_.end(), [&](auto const& existing) {
        return existing.sender_id == message.sender_id && existing.type == message.type;
    });
    if (found != reports_.end()) {
        if (found->round > message.round) {
            return;
        }
        *found = message;
    } else {
        if (reports_.size() >= 64U) {
            reports_.erase(std::min_element(reports_.begin(), reports_.end(), [](auto const& a, auto const& b) {
                return a.round < b.round;
            }));
        }
        reports_.push_back(message);
    }
    if (message.type == MessageType::portal) {
        auto const endpoint = PortalEndpoint{{message.x, message.y},
            (message.value & 1) == 0 ? unswbc::Direction::NORTH : unswbc::Direction::WEST};
        auto const portal_id = message.value / 2;
        auto const& local = cell(endpoint.position).edges[geometry::direction_index(endpoint.direction)];
        auto const* ends = portal_endpoints(portal_id);
        if ((!local.seen || (local.type == unswbc::EdgeType::PORTAL && local.portal_id == portal_id))
            && (ends == nullptr || ends->size() < 2U)) {
            remember_portal(portal_id, endpoint);
        }
    }
    // Reports never mark a tile seen, overwrite local occupancy, or certify an
    // empty portal exit. Delayed echoes also cannot establish those facts.
}

auto WorldModel::reports() const -> std::vector<TeamMessage> const& { return reports_; }

auto WorldModel::queen_reservations(unswbc::Controller const& controller, int round) const
    -> std::vector<int> {
    auto reserved = std::vector<int>(cells_.size(), 0);
    if (!config::enable_queen_corridors || controller.get_id() <= 1) { return reserved; }
    for (auto const& report : reports_) {
        if (report.type == MessageType::danger && report.sender_id <= 1 && round - report.round <= 1) {
            reserved[index({report.x, report.y})] = config::enable_sonar_network ? 120000 : 12000;
        }
    }
    for (auto const& tile : controller.get_tiles()) {
        auto const* queen = tile.get_dragon();
        if (queen == nullptr || !queen->is_head() || queen->get_id() > 1
            || queen->get_team() != controller.get_team()) { continue; }
        auto exits = std::vector<std::pair<unswbc::Position, unswbc::Direction>>{};
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const p = transition(tile.get_position(), direction);
            auto const* next = p ? controller.get_tile(*p) : nullptr;
            if (next != nullptr && (next->get_dragon() == nullptr
                || next->get_dragon()->get_id() == controller.get_id())) {
                exits.emplace_back(*p, direction);
            }
        }
        for (auto const& [p, direction] : exits) {
            auto& weight = reserved[index(p)];
            weight = std::max(weight, exits.size() == 1U ? 120000 : 16000);
            // Reserve a continuation, not a whole area around the queen.
            auto const onward = transition(p, direction);
            auto const* next = onward ? controller.get_tile(*onward) : nullptr;
            if (next != nullptr && next->get_dragon() == nullptr) {
                reserved[index(*onward)] = std::max(reserved[index(*onward)], 8000);
            }
        }
    }
    return reserved;
}

auto WorldModel::queen_intent(unswbc::Controller const& controller, int round,
                              PlannedAction const& action) const -> std::optional<TeamMessage> {
    if (!config::enable_queen_corridors || controller.get_id() > 1) { return std::nullopt; }
    auto const simulation = Simulation{};
    auto state = simulation.initial_state(controller, this);
    if (action.kind == ActionKind::split) {
        if (!controller.can_split(action.split_size)) { return std::nullopt; }
        state.unranked_body.assign(state.body.end() - action.split_size, state.body.end());
        state.body.resize(state.body.size() - static_cast<std::size_t>(action.split_size));
    } else {
        for (std::size_t i = 0; i < action.steps.size(); ++i) {
            auto const next = simulation.advance(controller, state, action.steps[i], i > 0, this);
            if (!next) { return std::nullopt; }
            state = *next;
        }
    }
    auto best = std::optional<unswbc::Position>{};
    auto best_score = -1;
    for (auto const direction : unswbc::Direction::get_direction_list()) {
        auto const next = simulation.advance(controller, state, direction, false, this);
        if (!next) { continue; }
        auto budget = 64;
        auto const score = simulation.survival_depth(controller, *next, 4, budget, this) * 100
            + next->pearls * 10 + (direction == controller.get_dir() ? 1 : 0);
        if (score > best_score) { best = next->body.front(); best_score = score; }
    }
    if (!best) { return std::nullopt; }
    return TeamMessage{MessageType::danger, round & 511, controller.get_id(), best->x, best->y, 0};
}

auto WorldModel::portal_hazard(unswbc::Position exit, int portal_id, int round) const -> bool {
    auto newest_warning = -1;
    auto newest_productive = -1;
    for (auto const& report : reports_) {
        auto const age = round - report.round;
        if (report.type != MessageType::empty || report.sender_id <= 1 || age < 0 || age > 4
            || report.x != exit.x || report.y != exit.y || (report.value & 1023) != portal_id) { continue; }
        if (report.value >= 1024) { newest_productive = std::max(newest_productive,report.round); }
        else { newest_warning = std::max(newest_warning,report.round); }
    }
    return newest_warning >= 0 && newest_warning >= newest_productive;
}

auto WorldModel::portal_warning(unswbc::Controller const& controller, int round) const -> std::optional<TeamMessage> {
    if (controller.get_id() <= 1) { return std::nullopt; }
    for (auto const& tile : controller.get_tiles()) {
        auto const p = tile.get_position();
        if (tile.get_dragon() != nullptr) { continue; }
        auto portal_id = -1;
        for (auto const d : unswbc::Direction::get_direction_list()) {
            auto const& edge = tile.get_edge(d);
            if (edge.is_portal() && edge.get_portal_id() >= 0 && edge.get_portal_id() < 1024) {
                portal_id = edge.get_portal_id(); break;
            }
        }
        if (portal_id < 0) { continue; }
        auto onward = 0;
        auto seen = 0;
        for (auto const d : unswbc::Direction::get_direction_list()) {
            // Count normal onward corridors rather than the portal back out.
            if (tile.get_edge(d).is_portal()) { continue; }
            auto const next = transition(p,d);
            auto const* landing = next ? controller.get_tile(*next) : nullptr;
            seen += !tile.get_edge(d).is_passable() || landing != nullptr;
            onward += landing != nullptr && landing->get_dragon() == nullptr;
        }
        auto enemy_near = false;
        for (auto const& other : controller.get_tiles()) {
            auto const* part = other.get_dragon();
            enemy_near = enemy_near || (part != nullptr && part->is_head()
                && part->get_team() != controller.get_team()
                && geometry::toroidal_manhattan(p,other.get_position(),width_,height_) <= 4);
        }
        if ((seen >= 3 && onward < 2) || enemy_near) {
            return TeamMessage{MessageType::empty,round,controller.get_id(),p.x,p.y,portal_id};
        }
    }
    return std::nullopt;
}

auto WorldModel::portal_survey(unswbc::Controller const& controller, int round,
                               PlannedAction const& action) const -> std::optional<TeamMessage> {
    auto path = std::vector<unswbc::Position>{};
    auto position = controller.get_position();
    if (action.kind != ActionKind::split) {
        for (auto const direction : action.steps) {
            auto const next = transition(position, direction);
            if (!next) { return std::nullopt; }
            position = *next;
            path.push_back(position);
        }
    }
    auto best = std::optional<TeamMessage>{};
    auto best_score = -1;
    auto survey_candidates = 0;
    auto const& tiles = controller.get_tiles();
    for (std::size_t offset = 0; offset < tiles.size(); ++offset) {
        auto const& tile = tiles[(offset + static_cast<std::size_t>(round / 4 + controller.get_id())) % tiles.size()];
        auto const p = tile.get_position();
        if (tile.get_dragon() != nullptr || p == controller.get_position()
            || std::find(path.begin(), path.end(), p) != path.end()) { continue; }
        auto portal_id = -1;
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const& edge = tile.get_edge(direction);
            if (edge.is_portal() && edge.get_portal_id() >= 0 && edge.get_portal_id() < 1024) {
                portal_id = edge.get_portal_id(); break;
            }
        }
        if (portal_id < 0) { continue; }
        auto onward = 0;
        for (auto const direction : unswbc::Direction::get_direction_list()) {
            auto const next = transition(p, direction);
            auto const* visible = next ? controller.get_tile(*next) : nullptr;
            onward += visible != nullptr && visible->get_dragon() == nullptr && *next != p;
        }
        if (onward < 2) { continue; }
        auto enemy_near = false;
        for (auto const& observed : controller.get_tiles()) {
            auto const* part = observed.get_dragon();
            if (part != nullptr && part->is_head() && part->get_team() != controller.get_team()
                && geometry::toroidal_manhattan(p, observed.get_position(), width_, height_) <= 4) {
                enemy_near = true; break;
            }
        }
        if (enemy_near) { continue; }
        if (survey_candidates++ >= 4) { break; }
        auto queue = std::vector<std::pair<unswbc::Position,int>>{{p,0}};
        auto resources = 0;
        for (std::size_t cursor = 0; cursor < queue.size() && cursor < 32U; ++cursor) {
            auto const [current, distance] = queue[cursor];
            auto const* visible = controller.get_tile(current);
            if (visible == nullptr) { continue; }
            resources += visible->has_pearl() || (visible->get_pearl_time() > 0 && visible->get_pearl_time() <= 12);
            if (distance == 3) { continue; }
            for (auto const direction : unswbc::Direction::get_direction_list()) {
                auto const next = transition(current, direction);
                auto const* target = next ? controller.get_tile(*next) : nullptr;
                if (target != nullptr && target->get_dragon() == nullptr
                    && std::none_of(queue.begin(), queue.end(), [&](auto const& entry) { return entry.first == *next; })) {
                    queue.emplace_back(*next, distance + 1);
                }
            }
        }
        auto const score = resources * 100 + onward * 10;
        if (score > best_score) {
            best_score = score;
            // This is a recent observation, not a promise that an exit remains empty.
            best = TeamMessage{MessageType::empty, round & 511, controller.get_id(), p.x, p.y,
                portal_id + (resources > 0 ? 1024 : 0)};
        }
    }
    return best;
}

auto WorldModel::own_body(unswbc::Controller const& controller) const
    -> std::vector<unswbc::Position> const* {
    if (own_id_ != controller.get_id() || own_body_.size() != static_cast<std::size_t>(controller.get_length())
        || own_body_.empty() || own_body_.front() != controller.get_position()) {
        return nullptr;
    }
    for (std::size_t i = 1; i < own_body_.size(); ++i) {
        auto const p = own_body_[i];
        if (p.x < 0 || p.y < 0) {
            continue;
        }
        if (auto const* tile = controller.get_tile(p)) {
            auto const* part = tile->get_dragon();
            if (part == nullptr || part->get_id() != controller.get_id() || part->is_head()) {
                return nullptr;
            }
        }
    }
    return &own_body_;
}

auto WorldModel::reconcile_body(unswbc::Controller const& controller, unswbc::Game const& game) -> void {
    auto candidate = std::vector<unswbc::Position>{};
    if (own_id_ == controller.get_id() && pending_round_ + 1 == game.get_round_num() && !pending_body_.empty()) {
        if (pending_single_move_ && (controller.get_length() == static_cast<int>(pending_body_.size())
            || controller.get_length() == static_cast<int>(pending_body_.size()) + 1)) {
            // The next observation confirms the actual landing tile and growth,
            // including a single uncertain portal crossing.
            candidate.push_back(controller.get_position());
            candidate.insert(candidate.end(), pending_body_.begin(), pending_body_.end());
            candidate.resize(static_cast<std::size_t>(controller.get_length()));
        } else if (!pending_single_move_ && pending_body_.size() == static_cast<std::size_t>(controller.get_length())
            && pending_body_.front() == controller.get_position()) {
            candidate = pending_body_;
        }
    }
    pending_body_.clear();
    pending_single_move_ = false;
    auto valid = !candidate.empty();
    for (std::size_t i = 1; valid && i < candidate.size(); ++i) {
        auto const p = candidate[i];
        if (p.x < 0 || p.y < 0) {
            continue;
        }
        auto const* tile = controller.get_tile(p);
        auto const* part = tile != nullptr ? tile->get_dragon() : nullptr;
        if ((tile != nullptr && (part == nullptr || part->get_id() != controller.get_id() || part->is_head()))
            || std::find(candidate.begin(), candidate.begin() + static_cast<std::ptrdiff_t>(i), p)
                != candidate.begin() + static_cast<std::ptrdiff_t>(i)) {
            valid = false;
        }
        if (part != nullptr && candidate[i-1].x >= 0) {
            auto const ahead = transition(p, part->get_dir());
            valid = valid && (!ahead || *ahead == candidate[i-1]);
        }
    }
    if (valid && std::none_of(candidate.begin(), candidate.end(), [](auto p) { return p.x < 0 || p.y < 0; })) {
        for (auto const& tile : controller.get_tiles()) {
            auto const* part = tile.get_dragon();
            if (part != nullptr && part->get_id() == controller.get_id()
                && std::find(candidate.begin(), candidate.end(), tile.get_position()) == candidate.end()) {
                valid = false;
            }
        }
    }
    own_body_ = valid ? std::move(candidate) : std::vector<unswbc::Position>{};
    own_id_ = controller.get_id();
    own_body_ = Simulation{}.initial_state(controller, this).body;
}

auto WorldModel::remember_action(unswbc::Controller const& controller, unswbc::Game const& game,
                                 PlannedAction const& action) -> void {
    pending_body_.clear();
    pending_single_move_ = false;
    pending_round_ = game.get_round_num();
    auto const simulation = Simulation{};
    auto state = simulation.initial_state(controller, this);
    if (action.kind == ActionKind::split) {
        if (controller.can_split(action.split_size)) {
            state.body.resize(state.body.size() - static_cast<std::size_t>(action.split_size));
            pending_body_ = std::move(state.body);
        }
    } else if (action.steps.size() == 1) {
        pending_single_move_ = true;
        pending_body_ = std::move(state.body);
    } else {
        for (std::size_t i = 0; i < action.steps.size(); ++i) {
            auto next = simulation.advance(controller, state, action.steps[i], i > 0, this);
            if (!next) {
                return;
            }
            state = std::move(*next);
        }
        pending_body_ = std::move(state.body);
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

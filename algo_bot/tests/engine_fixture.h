#ifndef SUDO_WIN_TESTS_ENGINE_FIXTURE_H
#define SUDO_WIN_TESTS_ENGINE_FIXTURE_H

#include "sudo_win/engine/helper.h"

#include <vector>
#include <stdexcept>

namespace sudo_win::test {

class EngineFixture {
public:
    EngineFixture()
    : game{10, 10, 64}
    , controller{0, unswbc::Team::A, unswbc::Direction::NORTH, make_vision(), 64} {
        controller.head.position = {5, 5};
        unswbc::game = &game;
        unswbc::ct = &controller;
    }

    EngineFixture(EngineFixture const&) = delete;
    auto operator=(EngineFixture const&) -> EngineFixture& = delete;

    ~EngineFixture() {
        unswbc::ct = nullptr;
        unswbc::game = nullptr;
    }

    unswbc::Game game;
    unswbc::Controller controller;

    [[nodiscard]] auto tile(unswbc::Position position) -> unswbc::Tile& {
        auto* result = controller.get_tile(position);
        if (result == nullptr) {
            throw std::out_of_range{"test fixture position is outside its vision"};
        }
        return *result;
    }

private:
    [[nodiscard]] static auto make_vision() -> unswbc::Vision {
        auto tiles = std::vector<unswbc::Tile>{};
        tiles.reserve(49);
        for (auto y = 2; y <= 8; ++y) {
            for (auto x = 2; x <= 8; ++x) {
                tiles.emplace_back(unswbc::Position{x, y});
            }
        }
        return unswbc::Vision{std::move(tiles)};
    }
};

} // namespace sudo_win::test

#endif // SUDO_WIN_TESTS_ENGINE_FIXTURE_H

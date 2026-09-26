#ifndef SUDO_WIN_TESTS_TEST_SUPPORT_H
#define SUDO_WIN_TESTS_TEST_SUPPORT_H

#include <iostream>
#include <string_view>

namespace sudo_win::test {

class TestSuite {
public:
    auto expect(bool condition, std::string_view message) -> void {
        ++assertions_;
        if (condition) {
            return;
        }
        ++failures_;
        std::cerr << "FAILED: " << message << '\n';
    }

    [[nodiscard]] auto finish() const -> int {
        if (failures_ == 0) {
            std::cout << assertions_ << " assertions passed\n";
            return 0;
        }
        std::cerr << failures_ << " of " << assertions_ << " assertions failed\n";
        return 1;
    }

private:
    int assertions_ = 0;
    int failures_ = 0;
};

} // namespace sudo_win::test

#endif // SUDO_WIN_TESTS_TEST_SUPPORT_H

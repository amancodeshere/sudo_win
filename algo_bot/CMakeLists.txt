cmake_minimum_required(VERSION 3.19)

execute_process(
    COMMAND bash util/setup.sh
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY
)

project(SUDO_WIN_ALGO_BOT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -Wall
    -Wextra
    -Werror
    -pedantic-errors
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wcast-align
    -Wformat=2
    -Wuninitialized
    -Wnull-dereference
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wdeprecated-copy-dtor
    -Wold-style-cast
    -Wzero-as-null-pointer-constant
    -Wsuggest-override
    -fstack-protector-strong
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(SANITIZER_FLAGS "$<$<CONFIG:Debug,RelWithDebInfo>:-fsanitize=address,undefined>")
add_compile_options("${SANITIZER_FLAGS}")
add_link_options("${SANITIZER_FLAGS}")

set(SUDO_WIN_SOURCES
    src/bot/bot.cpp
    src/combat/combat.cpp
    src/economy/economy.cpp
    src/endgame/endgame.cpp
    src/geometry/geometry.cpp
    src/pathfinding/pathfinding.cpp
    src/planner/planner.cpp
    src/roles/roles.cpp
    src/safety/safety.cpp
    src/sonar/sonar.cpp
    src/splitting/splitting.cpp
    src/world/world_model.cpp
)

add_library(sudo_win_bot STATIC ${SUDO_WIN_SOURCES})
target_include_directories(sudo_win_bot PUBLIC include)

add_executable(sudo_win src/main.cpp)
target_link_libraries(sudo_win PRIVATE sudo_win_bot)

enable_testing()

add_executable(
    sudo_win_tests
    tests/test_main.cpp
    tests/combat/combat_test.cpp
    tests/economy/economy_test.cpp
    tests/endgame/endgame_test.cpp
    tests/geometry/geometry_test.cpp
    tests/pathfinding/pathfinding_test.cpp
    tests/planner/planner_test.cpp
    tests/roles/roles_test.cpp
    tests/safety/safety_test.cpp
    tests/sonar/sonar_test.cpp
    tests/splitting/splitting_test.cpp
    tests/world/world_model_test.cpp
)
target_include_directories(sudo_win_tests PRIVATE tests)
target_link_libraries(sudo_win_tests PRIVATE sudo_win_bot)

add_test(NAME sudo_win_tests COMMAND sudo_win_tests)

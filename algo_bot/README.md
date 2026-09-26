# Algorithmic Bot

C++20 UNSW Battlecode bot organised as a feature-based C++ project.

## Layout

```text
algo_bot/
├── CMakeLists.txt
├── bot.toml
├── helper.hpp
├── lib/
│   ├── catch2/catch.hpp
│   └── catch2_main.cpp
├── include/sudo_win/
│   ├── bot/
│   ├── combat/
│   ├── config/
│   ├── economy/
│   ├── endgame/
│   ├── engine/
│   ├── geometry/
│   ├── pathfinding/
│   ├── planner/
│   ├── roles/
│   ├── safety/
│   ├── sonar/
│   ├── splitting/
│   ├── types/
│   └── world/
├── src/<feature>/
├── tests/<feature>/
└── util/setup.sh
```

Public headers use include guards, trailing return types, `[[nodiscard]]` on
queries, scoped namespaces, and consistent brace initialization.

## Toolkit

Install the official toolkit and refresh its generated helper when the protocol
changes:

```bash
uv tool install unswbc
unswbc update algo_bot
unswbc maps
```

Review helper changes before committing them.

## Build and Test

Keep build output outside the source tree:

```bash
cmake -S algo_bot -B /tmp/sudo-win-build -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/sudo-win-build
ctest --test-dir /tmp/sudo-win-build --output-on-failure
```

The CMake build enables strict warnings, treats warnings as errors, enables
AddressSanitizer and UndefinedBehaviorSanitizer in debug-style builds, and runs
the Catch2 test suite through CTest.

## Continuous Integration

GitHub Actions runs the complete Catch2 suite with both GCC and Clang for every
pull request, plus every push that changes `algo_bot` or its workflow. Both jobs
use the Debug configuration, so AddressSanitizer and UndefinedBehaviorSanitizer
fail the workflow when they detect a runtime error.

Pushes to `main` and manual workflow runs also build and test the Release
configuration, then upload a 14-day artifact containing the Linux executable
and the submission sources. The delivery job runs only after both compiler test
jobs pass. It does not submit the bot to the competition server.

Before merging, require the following checks in the repository's branch
protection settings:

- `Algorithm Bot CI / GCC Debug + sanitizers`;
- `Algorithm Bot CI / Clang Debug + sanitizers`.

## Run a Match

```bash
unswbc run --sandbox -v maps/arena.map algo_bot algo_bot
```

Always use `--sandbox` for performance checks. Ordinary local runs do not apply
the judge's CPU-point budget.

## Baseline

The current implementation provides persistent visible map memory, conservative
collision avoidance, flood-fill mobility, pearl scoring, elementary roles,
combat-risk penalties, authenticated sonar encoding, and a mandatory fallback.
Unknown portal exits, sprints, splits, and sonar transmission remain disabled by
default in `include/sudo_win/config/config.h`.

See `IMPLEMENTATION_GUIDE.md` for the complete baseline-to-endgame roadmap.

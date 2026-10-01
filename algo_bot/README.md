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
uv tool install unswbc==1.2.2
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
and the submission source ZIP with a checksum manifest. Delivery requires both
compiler jobs and the packaging/deterministic judge-sandbox job. It does not
submit the bot to the competition server.

Before merging, require the following checks in the repository's branch
protection settings:

- `Algorithm Bot CI / GCC Debug + sanitizers`;
- `Algorithm Bot CI / Clang Debug + sanitizers`.
- `Algorithm Bot CI / Submission tools + judge sandbox`.

## Run a Match

```bash
python3 algo_bot/util/prepare_submission.py algo_bot --output /tmp/sudo-win-submission
unswbc run --sandbox -v --seed 1 maps/arena.map /tmp/sudo-win-submission /tmp/sudo-win-submission
```

Always use `--sandbox` for performance checks. Ordinary local runs do not apply
the judge's CPU-point budget.

The current toolkit's sandbox compiler scans every C++ file under the directory
passed to it. Use a clean submission directory so it does not compile Catch2 and
the test entry point alongside the bot. The benchmark tool stages selected files
automatically.

## Reproducible Benchmarks

Install the pinned match dependency into a virtual environment:

```bash
uv venv /tmp/sudo-win-bench-env
uv pip install --python /tmp/sudo-win-bench-env/bin/python -r algo_bot/util/requirements.txt
/tmp/sudo-win-bench-env/bin/python -m unittest discover -s algo_bot/util/tests
```

Preserve the old submission sources as a separate bot directory, then compare:

```bash
XDG_CACHE_HOME=/tmp/sudo-win-cache /tmp/sudo-win-bench-env/bin/python \
  algo_bot/util/benchmark.py algo_bot /tmp/sudo-win-baseline \
  --seeds 1 2 3 --both-colours --repeat 2 --output /tmp/sudo-win-results
```

With no `--maps`, every map bundled with toolkit 1.2.2 is used. Narrow iteration
with `--maps maps/arena.map`. Runs use the judge sandbox by default and compile
only manifest-selected files, excluding Catch2 and test entry points. Each
output directory must be fresh. JSONL records include source/map fingerprints,
colour, seed, winner, final team total lengths, deaths, errors, CPU p50/p95/max,
peak observed lengths, and replay hashes. Repeated replays must match exactly.
The command fails on runtime errors, no-valid-action deaths, nondeterminism, or
turns exceeding the default 90-million-point margin. It also independently checks
death snapshots for visible body/wall collisions or unaffordable sprints when a
safe ordinary alternative existed. Deaths with no known safe alternative and
future enemy attacks remain recorded, rather than assumed avoidable. Action
counts distinguish ordinary moves, sprints, and splits.

`--native` is a faster diagnostic mode and cannot verify CPU budgets. Peak
observed length is sampled before actions; it is not final longest-dragon length.
The engine still determines wins using its actual scoring rules.

## Prepare an Upload

```bash
python3 algo_bot/util/prepare_submission.py algo_bot \
  --profile stable --output build/submission-stable
```

This produces a clean bot directory, `build/submission-stable.zip`, and a JSON
checksum/flag manifest. Choose a fresh output name for later builds. Profiles
are applied to the copy, leaving source configuration unchanged:

- `stable`: configured sprint policy, splitting/sonar/indicators disabled;
- `no-sprint`: stable strategy with sprint generation disabled for ablations;
- `experimental`: enables the resource-backed split policy.

Test the resulting directory through `benchmark.py` before uploading. If the
toolkit is already authenticated, `unswbc submit build/submission-stable` submits
the sources. Packaging and CI do not submit automatically.

## Download Competition Replays

From the repository root, load your git-ignored API credentials and download
available games into `replays/`:

```bash
set -a
source .env
set +a
python3 algo_bot/util/download_replays.py
```

The downloader expands each series into individual games, preserves existing
replays, and saves battle results plus a download summary alongside the files.
Rerun it to fetch new games. The API returns at most 200 recent series; reaching
that limit produces a warning rather than claiming the entire history was saved.
Open `.replay` files in the VS Code Battledragon Replay viewer.

After installing `util/requirements.txt`, audit every downloaded replay with:

```bash
python3 algo_bot/util/analyze_replays.py --output build/replay-audit
```

This verifies reconstructed final standings and exports per-game death evidence,
CPU measurements, maps, and compressed visible turn inputs. See `REPLAY_AUDIT.md`
for the review of our first 75 competition games.

## Current Strategy

The stable bot uses persistent map memory, remembered pearl/frontier routing,
target hysteresis, six-step body-aware survival search, remembered escape-space
penalties, legal enemy move prediction, local champion estimates, gradual endgame
caution, and validated two-/three-step sprints. Known portal exits must be visible
and empty before execution. Search has fixed node budgets; sandbox evaluation
checks the judge's actual CPU points.

Resource-backed splitting is implemented but remains experimental and disabled
by default. Sonar reports are decoded but still not applied or transmitted.
The competition build has a planning exception fallback; Debug/RelWithDebInfo
builds expose exceptions to catch development errors.

See `VALIDATION.md` for measured results and `IMPLEMENTATION_GUIDE.md` for the
remaining coordination, tactical search, and endgame roadmap.

# Submission validation — 1 October 2026

The stable submission enables body-aware survival search, remembered-map routing,
closed-pocket penalties, legal enemy threat estimates, verified visible portal
exits, local champion estimates, and short sprints. Splitting, sonar transmission,
and indicators remain disabled.

## Held-out comparison

The stable sources played the original repository bot from commit `c07228f` on
all 15 maps bundled with `unswbc==1.2.2`, using seeds `31 32 33 34 35` and both
colours. The original source snapshot was preserved before implementation work.

| Measure | Result |
| --- | --- |
| Games | 150 |
| Wins / losses / draws | 102 / 48 / 0 |
| Win rate against original bot | 68% |
| Runtime/action-format errors | 0 |
| Independently detected avoidable visible collisions | 0 |
| No-valid-action deaths | 0 |
| Peak CPU points per turn | 42,019,717 |
| Judge turn limit / benchmark promotion margin | 100,000,000 / 90,000,000 |
| Candidate ordinary moves / sprints | 79,981 / 1,779 |

The independent collision audit examines death-turn observations, terrain,
visible bodies, known portal transitions, and sprint payment ordering. Uncertain
body ranks and unseen portal destinations are not treated as proof of safety or
fatality. It does not certify long-term survival or predict every enemy action.
Fatal fallbacks with no known safe ordinary alternative are recorded rather than
mislabelled avoidable.

| Map | Wins / losses |
| --- | --- |
| Colosseum | 8 / 2 |
| Arena | 7 / 3 |
| Autarky | 8 / 2 |
| Big Empty | 7 / 3 |
| Default | 5 / 5 |
| Default Small | 7 / 3 |
| Devil | 5 / 5 |
| Dilemma | 10 / 0 |
| Portals | 5 / 5 |
| Queen of Spades | 6 / 4 |
| Schooltime | 8 / 2 |
| Slithery Fight | 10 / 0 |
| Stronghold | 6 / 4 |
| Trauma | 3 / 7 |
| Trophy | 7 / 3 |

This is evidence of improvement against the original bot, not a tournament win
prediction. Trauma remains a weak matchup. Five distinct seeds per map do not
meet the roadmap's larger 100-seed-per-map gate, and stronger saved opponents
are still needed. Results on nearly deterministic maps are not independent
evidence of many different situations.

## Feature checks

Each feature was checked with the complete C++ suite and sandbox matches before
its commit. Selected promotion comparisons:

| Comparison | Games | Wins / losses / draws | Decision |
| --- | --- | --- | --- |
| Sprints enabled vs otherwise identical disabled variant | 32 | 20 / 12 / 0 | Retain sprints |
| Remembered escape-space + deeper survival vs previous strategy | 30 | 18 / 12 / 0 | Retain escape-space scoring |
| Experimental splitting vs disabled variant | 8 | 4 / 4 / 0 | Keep splitting disabled |

The split investment fixture additionally exercised a real legal three-segment
split, fresh child turns, and ordinary/sprint continuation in the judge sandbox.
One successful fixture does not establish strategic value.

Routing's earlier isolated comparison was 6 wins / 9 losses / 1 draw in 16
games against the initial lookahead version. That feature alone did not establish
a benefit; the reported 68% result is for the integrated final strategy.

## Build and package checks

- Clang 21 Debug with AddressSanitizer/UndefinedBehaviorSanitizer: passes.
- Clang 21 Release: passes.
- GCC 15 Release with strict warnings as errors: passes.
- C++ suite: 28 test cases, 170 assertions.
- Python benchmark/packaging suite: 5 tests, passes.
- Workflow YAML parsing and delivery dependencies: validated locally.
- Exact stable submission directory: four repeated judge-sandbox self-matches
  on Arena, seed 101, both colours; replay hashes match within each matchup.
- ZIP integrity, source exclusion, isolated flags, and deterministic packaging:
  checked by the Python suite.

GCC Debug could not link on this Mac because Homebrew GCC lacks the `asan`
runtime. Clang performed the local sanitizer checks. Ubuntu GCC and Clang
sanitizer jobs remain configured in CI; updated remote CI has not been run from
this local session.

## Artifacts and reproduction

The local stable package is `build/submission-stable.zip`, with its selected
sources in `build/submission-stable/` and checksums in
`build/submission-stable.json`. The benchmark source fingerprint is
`7873c84a5ad7ddd54dc7568de37de34d8d0192b050dbd6a123922f404e99388a`.
The packaging manifest hashes the selected sources plus `bot.toml`, so its
source checksum uses a different format.

Raw JSONL records, death snapshots, and replays are preserved locally under
`build/validation/`; they are ignored build artifacts. Use `candidate_team` in
JSONL to identify the tested side, particularly in older experiment replays.
Final delivery replays label both sides accurately. Team total lengths in the
JSONL are not the longest-dragon metric; the actual engine decides the winner.

Reproduce the comparison after restoring the old source snapshot:

```bash
mkdir -p /tmp/sudo-win-original
git archive c07228f:algo_bot | tar -x -C /tmp/sudo-win-original
XDG_CACHE_HOME=/tmp/sudo-win-cache /tmp/sudo-win-tools/unswbc/bin/python \
  algo_bot/util/benchmark.py build/submission-stable /tmp/sudo-win-original \
  --seeds 31 32 33 34 35 --both-colours --output /tmp/sudo-win-heldout-rerun
```

The `/tmp/sudo-win-tools/unswbc/bin/python` environment used here has toolkit
1.2.2 and Wasmtime 49.0.0. For a fresh machine, create the environment from
`util/requirements.txt` using the README instructions and substitute its Python.

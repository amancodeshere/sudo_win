# Submission validation

## Replay-driven candidate — 2 October 2026

The competition's original submission (version 14151) went 24–51 over 75 games.
Every downloaded replay was audited; reconstructed final standings match the
recorded results for both teams in every game. See `REPLAY_AUDIT.md` for the
per-game review and `build/replay-audit/` for full death traces and observations.

The promoted changes are legal rescue splits, allied-head protection during
forced losses, and one cached enemy threat map per turn. Rescue splitting is
independent of the disabled experimental investment policy. No new submission
has been uploaded or activated during this work.

| Verification | Result |
| --- | --- |
| Rescue vs exact deployed bot, all 15 maps, seeds 61/62/63, both colours | 73 wins / 17 losses; native diagnostics |
| Rescue vs deployed bot, 3 trap maps, seeds 61/62, both colours | 9 wins / 3 losses; judge sandbox |
| Optimized candidate vs deployed bot, all 15 maps, held-out seed 81, both colours | 27 wins / 3 losses; judge sandbox |
| Optimized candidate maximum CPU in that run | 37,234,819 / 100,000,000 points |
| Whole match maximum including deployed opponent | 45,526,098 points |
| Exact delivered ZIP, Slithery Fight, seed 82, both colours, each repeated | 2 wins; 4 runs with identical repeat hashes |
| Exact ZIP run maximum including opponent | 36,536,192 points |
| Cached vs uncached protected-rescue bot on stored observations | 55,275 turns across 75 games; zero action differences or errors |
| C++ tests | 31 cases, 180 assertions; Apple Clang Release and ASan/UBSan Debug, LLVM Clang 22 Release, GCC 15 Release |
| Python utility tests | 8 passed |
| Invalid actions, avoidable visible collisions, timeouts in promoted sandbox runs | None |

The all-map run used the optimized source before the final explicit split-head
null guards required by strict GCC. The guards are covered by all compiler/test
builds, and the exact final source archive passed the repeated sandbox package
check. The cached behavior comparison predates those guards and compares native
executables, not CPU readings or revised match outcomes.

Experiments were evaluated against the stronger rescue bot before promotion:

| Trial | Outcome | Decision |
| --- | --- | --- |
| Friendly head movement treated as combat threats | 5–7 over 12 native games | Reject |
| Ten-step search with remembered terrain and increased node budget | 11–13 over 24 native games; 1–3 sandbox subset | Reject |
| Broad five-step sprint threats with hard escape priority | 6–14 over 20 native games; 1–3 sandbox subset | Reject |
| Observed-body affordability and reduced long-sprint penalties | 5–15 over 20 native games | Reject |
| Revisiting fresh beds and forecasting older spawn observations | 5–7 over 12 native games | Reject |

These comparisons are small controlled samples. They prevent promoting observed
regressions; they do not prove a variant is universally inferior. Saved evidence
is under `build/validation/replay-*`, including failed/incomplete initial native
runs. The successful complete rescue run used a higher process file-descriptor
limit because native multi-snake tests exhausted macOS's default limit.

Trauma remains weak: both held-out seed-81 games were losses. Autarky lost one
colour. Opponent source code is absent from competition replay downloads, so
changed bots cannot be tested against those exact adaptive opponents offline.
A single held-out seed per map is not a tournament-strength estimate. Stronger
live opponent testing is needed before claiming competition leadership.

### Delivered archive

- ZIP: `build/submission-replay-final.zip` (32,012 bytes).
- Selected sources: `build/submission-replay-final/`.
- Source-and-manifest SHA256: `fd27f7351a172c8e850cc9f670419e737889a1b2911e2ba9def34a2963a09ae5`.
- ZIP SHA256: `4c5cf257871bb1c40b08097cd1c98fd62cd97ba4b688382f7285692ca26f0cb9`.
- Final benchmark source-only SHA256: `c5093754c821b363b8d524abb844da84b3093b98ac6eea5d7c0ec3c6c4574194`.
- Original deployed source-only SHA256: `7873c84a5ad7ddd54dc7568de37de34d8d0192b050dbd6a123922f404e99388a`.

## Original 1 October 2026 package — historical

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

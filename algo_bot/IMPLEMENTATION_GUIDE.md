# Algorithmic Bot Implementation Guide

## 1. Design Goals

The bot should improve through independently measurable stages without forcing
large rewrites. Every turn follows this pipeline:

1. Parse the official controller state.
2. Update persistent local memory.
3. Decode sonar and update team knowledge.
4. Determine strategic phase and role.
5. Generate only affordable action candidates.
6. Reject candidates that are certainly fatal.
7. Score economy, mobility, combat, coordination, and endgame value.
8. Search future action sequences when the budget permits.
9. Emit exactly one final action and optional sonar.

Correctness comes before strategy. A weak living dragon is more useful than a
strong policy that occasionally outputs an illegal action.

## 2. Target File Structure

The project uses conventional `include`, `src`, `tests`, and `util` trees. Each
feature owns matching header, implementation, and test directories. The official
toolkit recursively collects files matched by `bot.toml`.

```text
algo_bot/
├── CMakeLists.txt              # strict local build and CTest configuration
├── bot.toml                    # competition submission manifest
├── helper.hpp                  # generated official protocol helper
├── include/sudo_win/
│   ├── bot/bot.h
│   ├── combat/combat.h
│   ├── config/config.h
│   ├── economy/economy.h
│   ├── endgame/endgame.h
│   ├── engine/helper.h         # wrapper around generated helper
│   ├── geometry/geometry.h
│   ├── pathfinding/pathfinding.h
│   ├── planner/planner.h
│   ├── roles/roles.h
│   ├── safety/safety.h
│   ├── sonar/sonar.h
│   ├── splitting/splitting.h
│   ├── types/types.h
│   └── world/world_model.h
├── src/
│   ├── main.cpp
│   └── <feature>/<feature>.cpp
├── tests/
│   ├── test_main.cpp
│   ├── engine_fixture.h
│   └── <feature>/<feature>_test.cpp
├── util/setup.sh
└── IMPLEMENTATION_GUIDE.md
```

The current scaffold tests combat, economy, endgame, geometry, pathfinding,
planning, roles, safety, sonar, splitting, and world memory. Add a diagnostics
feature after the baseline is stable so logging cannot hide timeout bugs.

`bot.toml` includes only `src`, public headers, and the generated helper. Tests,
CMake files, documentation, and utilities remain local and are not submitted to
the judge.

## 3. Build and Style Contract

All project headers use include guards rather than `#pragma once`. Public query
functions are marked `[[nodiscard]]`, function signatures use trailing return
types, and implementation state uses brace initialization and trailing
underscores for private members.

`CMakeLists.txt` builds the strategy as `sudo_win_bot`, links the competition
executable as `sudo_win`, and registers `sudo_win_tests` with CTest. Its warning
set treats conversions, formatting issues, null misuse, outdated copy
semantics, and other suspicious constructs as errors. Debug and RelWithDebInfo
builds enable AddressSanitizer and UndefinedBehaviorSanitizer.

Tests use a lightweight in-repository harness to avoid adding a dependency that
cannot ship in the competition sandbox. `tests/engine_fixture.h` constructs a
deterministic 7x7 observation window and safely installs the helper's global
game/controller pointers for feature tests.

## 4. Responsibilities by File

### `src/main.cpp`

Keep this file permanently small. It owns `unswbc::init`, the update loop,
`Bot::execute_turn`, and `unswbc::end_turn`. It must not contain strategy.

### `include/sudo_win/bot/bot.h` and `src/bot/bot.cpp`

Own long-lived per-dragon state. A split child starts a fresh process, so its
`Bot` begins with empty memory. Each turn this module should:

- update `WorldModel` before planning;
- decode valid sonar messages;
- choose the dragon's current role;
- invoke `Planner`;
- send queued sonar after selecting an action;
- emit the final move, sprint, or split;
- always emit a fallback action if planning fails.

Never catch logic errors and silently continue during development. For the
competition build, add a narrow emergency fallback around planning only.

### `include/sudo_win/config/config.h`

Put all tunable values here rather than scattering literals through strategy.
Maintain separate groups for:

- feature flags;
- phase boundaries;
- search depths and beam widths;
- split thresholds and population caps;
- score weights;
- safety margins and CPU-point budgets;
- sonar message expiry.

Create named configurations for stable, aggressive, and experimental variants.
Do not tune against only one opponent or map.

### `include/sudo_win/types/types.h`

Keep shared, engine-independent data structures here:

- `ActionKind`: move, sprint, split;
- `PlannedAction`: command, score, and reason;
- `MoveCandidate`: one-step action plus component scores;
- `Role`: champion, collector, scout, blocker, hunter;
- later, `StrategicPhase`, `Threat`, `Target`, and `SearchState`.

Search-state types should be value types with no references to controller
objects, allowing cheap copies and deterministic hashing.

### `include/sudo_win/geometry/geometry.h` and `src/geometry/geometry.cpp`

Implement all topology details once:

- direction-to-array-index conversion;
- coordinate wrapping;
- toroidal Manhattan and Chebyshev distance;
- shortest signed x/y displacement;
- symmetry transforms for x, y, and 180-degree symmetry;
- transition through a known portal endpoint;
- direction reconstruction between adjacent body segments.

All other modules should call geometry helpers instead of implementing their own
wrap logic.

### `include/sudo_win/world/world_model.h` and `src/world/world_model.cpp`

Maintain the dragon's persistent belief state. For every tile store:

- whether it has ever been seen;
- last-seen round;
- current pearl and pearl countdown from that observation;
- the four observed edge types and portal IDs;
- last observed occupant, team, ID, heading, and head/body status;
- confidence or age for any inferred information.

Also maintain:

- portal ID to the one or two observed endpoints;
- symmetry candidates: horizontal, vertical, and rotation;
- inferred mirrored tiles with lower confidence than observed tiles;
- recent enemy-head tracks;
- remembered high-value pearl regions;
- team reports received by sonar.

Never treat stale occupancy as current occupancy. Obstacles are permanent;
dragons are not.

### `include/sudo_win/safety/safety.h` and `src/safety/safety.cpp`

This is the hard legality layer. Strategy may rank safe actions but may not
override this module. It should eventually validate:

- every step in a normal move or sprint;
- kelp and portal transitions;
- collision with the body before tail advancement;
- intermediate sprint states and pearls eaten mid-sprint;
- sprint affordability before each additional step;
- legal split size and unit cap;
- visible head-on outcomes;
- uncertain exits beyond vision or through portals.

Return a structured result such as `Safe`, `Fatal`, or `Uncertain`, plus a
reason. The stable bot should reject `Uncertain`; aggressive configurations may
price it as risk.

### `include/sudo_win/pathfinding/pathfinding.h` and `src/pathfinding/pathfinding.cpp`

Build pathfinding in four layers:

1. Visible flood fill for immediate escape-space estimation.
2. BFS on remembered passable edges to known pearls and frontiers.
3. A* on the toroidal map with known portal transitions.
4. Time-expanded search that includes body release, pearl growth, and moving
   heads where enough information exists.

A path result must include distance, first direction, whether it crosses unknown
space, and the minimum reachable-space margin along the path. Do not blindly
follow a shortest path into a cul-de-sac.

### `include/sudo_win/economy/economy.h` and `src/economy/economy.cpp`

Value both present and future pearls. Target scoring should include:

- path distance and sprint segment cost;
- pearl count expected along the route;
- visible countdowns near expected arrival time;
- likelihood a spawn tile is blocked at countdown zero;
- enemy and ally competition;
- escape space after collection;
- death-drop opportunities;
- denial value when the opponent is likely to arrive first.

Remember that a sprint of `n` steps costs `n - 1` segments. A sprint collecting
one pearl per extra step is only length-neutral before tactical value.

### `include/sudo_win/roles/roles.h` and `src/roles/roles.cpp`

Start with deterministic roles so independent processes agree without shared
memory. Improve toward sonar-based assignments.

- **Champion:** preserve the team's likely longest dragon and avoid symmetric
  trades.
- **Collector:** maximise safe pearl income.
- **Scout:** reveal unseen edges, portal pairs, and symmetry evidence.
- **Blocker:** occupy routes and deny spawn areas without trapping allies.
- **Hunter:** pressure vulnerable enemy heads and collect death drops.

Role assignment should use length, ID, round, local density, known targets, and
recent team reports. Add hysteresis so a dragon does not change roles every
turn.

### `include/sudo_win/sonar/sonar.h` and `src/sonar/sonar.cpp`

Use a versioned 64-bit format. The scaffold reserves fields for message type,
round, sender ID, coordinates, value, and a keyed 16-bit tag. Expand message
types for:

- heartbeat and role claim;
- pearl target or rich region;
- portal endpoint;
- enemy head sighting;
- champion status;
- split announcement;
- feeder rendezvous;
- danger or blocked corridor.

Reject messages with an invalid tag, impossible coordinates, unknown version,
or expired round. Sonar carries no sender identity and can be intercepted by
either team; the payload must identify itself and must not be trusted without
validation.

Implement congestion control. Sending four rays every turn creates duplicates
and exposes information. Prefer event-driven messages with sequence numbers and
short expiry.

### `include/sudo_win/splitting/splitting.h` and `src/splitting/splitting.cpp`

Splitting is an economic investment, not a default growth action. Require:

- legal parent and child sizes;
- a team population below the configured cap;
- safe or sufficiently open regions at both head and tail;
- enough nearby pearl income for two dragons;
- no immediate endgame concentration requirement;
- no contested corridor where the child becomes free pearls.

Evaluate several child sizes rather than always splitting two. The new child
acts later in the same round with no inherited memory, so send any useful plan
before splitting when geometry allows and make child initialisation robust.

### `include/sudo_win/combat/combat.h` and `src/combat/combat.cpp`

First implement risk estimation, then tactical search:

- identify visible enemy heads, bodies, headings, IDs, and legal exits;
- distinguish enemies that already acted from those acting later this round;
- enumerate one-turn head-on and body-block outcomes;
- assign value to favourable trades and death-drop access;
- model ally congestion separately from enemy pressure;
- run local minimax or maximin against the few relevant enemy heads;
- use conservative opponent assumptions around unseen portal exits.

The evaluator should value survival nonlinearly. Trading a short hunter for a
long enemy can be correct; risking the champion for the same exchange is not.

### `include/sudo_win/endgame/endgame.h` and `src/endgame/endgame.cpp`

Activate endgame behaviour gradually rather than at one hard round:

- estimate the team's champion using local length and authenticated reports;
- protect the champion's reachable area and reserve escape corridors;
- stop low-value splitting and speculative sprints;
- move support dragons away from the champion's route;
- deny enemy champion resources when safe;
- preserve total living length if the longest lengths appear tied;
- arrange feeder rendezvous only when pearl recovery is likely.

For concentration, a feeder should die on a planned body geometry so alternating
segments become pearls along a route the champion can safely sweep. Account for
the roughly half-length conversion loss, the action time required, spawn
blocking, enemy theft, and the danger of trapping the champion. Never enable
this until ordinary endgame survival is reliable.

### `include/sudo_win/planner/planner.h` and `src/planner/planner.cpp`

The planner is the integration point, not the owner of domain rules.

Baseline scoring combines:

- reachable visible area;
- immediate and nearby pearl value;
- exploration frontier value;
- local enemy-head risk;
- reversal and congestion penalties;
- role-specific modifiers;
- endgame modifiers.

The final planner should:

1. Generate standard moves, affordable sprint sequences, and legal splits.
2. Ask `Safety` for legality and uncertainty.
3. Apply cheap filters and one-step scores.
4. Expand the best candidates with beam search.
5. Use a transposition table keyed by simulated body, pearls, and round.
6. Return the highest robust score with deterministic tie-breaking.

Reserve enough budget to emit a valid action even if search is stopped early.

## 5. Staged Implementation Plan

### Stage 0: Harness and Invariants

Implement before strategic work:

- install and pin the current `unswbc` toolkit;
- generate `helper.hpp` and official maps;
- add stable and experimental configuration profiles;
- run every test with `--sandbox -v`;
- record seed, map, colour, result, death cause, round, and points used;
- establish deterministic replay commands.

**Gate:** the same seed and bot pair produces identical results, and a batch can
be reproduced from one command.

### Stage 1: Safe Baseline

The scaffold begins this stage. Finish it by adding:

- explicit safety-result reasons;
- tests for wrap edges and own-tail collision;
- deterministic least-bad behaviour when every visible move is fatal;
- crash-safe action emission;
- point-budget counters with logging disabled by default.

**Gate:** zero malformed actions, illegal splits, oversprints, and avoidable
visible collisions across all official maps and at least 100 seeds.

### Stage 2: World Model and Pathfinding

Implement:

- persistent edge and pearl memory;
- symmetry-candidate elimination and inference;
- complete portal pairing and transition lookup;
- BFS to pearls and exploration frontiers;
- A* over the remembered toroidal graph;
- route invalidation when new observations contradict memory.

**Gate:** the bot explores every reachable region on empty-opponent tests,
crosses known-safe portals, and reaches known pearls without local oscillation.

### Stage 3: Pearl Economy

Implement:

- arrival-time-aware pearl target selection;
- competition estimates from visible heads and sightings;
- profitable sprint generation;
- spawn-tile occupancy management;
- death-drop collection and denial;
- anti-oscillation target reservations.

**Gate:** higher median maximum length and total length than Stage 2 across every
official map, with no material increase in deaths.

### Stage 4: Roles, Sonar, and Splitting

Implement:

- authenticated message encode/decode and expiry;
- event-driven sharing of portals, enemies, and targets;
- deterministic initial roles and stable reassignment;
- team target reservations;
- conservative split scoring and population caps;
- child bootstrap behaviour using ID, position, and any received message.

**Gate:** splitting improves team total length and map coverage against the
non-splitting bot while preserving or improving longest-dragon length.

### Stage 5: Tactical Combat and Search

Implement:

- enemy legal-move enumeration;
- turn-order-aware collision prediction;
- local maximin search around contested pearls;
- beam search for movement and bounded sprints;
- transposition tables and early budget cutoff;
- champion-specific risk scaling.

**Gate:** improved win rate against rush, greedy, blocker, and mirror opponents,
not merely against the previous bot.

### Stage 6: Endgame Concentration

Implement in this order:

1. Soft endgame risk increase around rounds 400–430.
2. Champion identification and heartbeats.
3. Support-dragon corridor clearing.
4. Enemy champion denial.
5. Total-length preservation when longest length is uncertain.
6. Feeder rendezvous planning.
7. Controlled death geometry and champion pearl sweep.

**Gate:** concentration must increase actual game wins, not just champion
length, over a large held-out seed set. Disable it on maps or states where pearl
recovery probability is poor.

## 6. Candidate Evaluation Model

Keep score components separate for ablations:

```text
score = survival
      + mobility
      + immediate_pearl
      + future_pearl
      + exploration
      + role_progress
      + combat_value
      + coordination
      + endgame_value
      - sprint_cost
      - uncertainty
      - congestion
      - trap_risk
```

Use integer scores for deterministic comparisons. Survival and certain death
should dominate all ordinary economic terms. Log component values only in local
debug builds.

## 7. Evaluation Matrix

For each candidate version run:

- every official map;
- both team colours;
- at least 20 fixed seeds during iteration;
- at least 100 held-out seeds before promotion;
- previous stable bot, greedy bot, rush bot, high-split bot, and current
  experimental bot as opponents.

Track:

- game win/draw/loss;
- maximum living length and total living length;
- survival round and engine death reason;
- pearls collected and pearls surrendered through death;
- successful and failed sprints;
- split count and child survival after 10 and 25 rounds;
- explored-map percentage and portal usage;
- sonar messages sent, accepted, rejected, and duplicated;
- median, p95, and maximum CPU points per turn.

Promote a version only when it improves aggregate win rate and does not create a
serious map-specific regression. Preserve named benchmark versions instead of
testing only against the latest build.

## 8. Immediate Next Tasks

1. Generate `helper.hpp` with `unswbc update algo_bot`.
2. Compile and run the scaffold against itself on `arena.map`.
3. Complete Stage 1 invariants and batch testing.
4. Implement symmetry inference and known-portal traversal.
5. Replace visible pearl scoring with remembered-map BFS targets.
6. Add profitable two- and three-step sprint candidates.
7. Add sonar only after single-dragon decisions are stable.

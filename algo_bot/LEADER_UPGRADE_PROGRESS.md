# Leader-driven upgrades

Baseline: `92667ed`, frozen v5 in `build/submission-competition-v5`.
Evidence: `LEADER_GAMEPLAY_REVIEW.md`. Current 17 map hashes are in the associated analysis JSON.

| Priority | Work | State | Evidence |
| --- | --- | --- | --- |
| 1 | Queen corridors and split-child yielding | Complete | 57 C++ cases / 369 assertions, release + ASan/UBSan; 16 Python tests; native 11–5 against v5 over 16 games, no errors |
| 2 | Earlier territorial expansion | Complete | Release + ASan/UBSan pass 57 cases / 373 assertions; native 5–3 over eight current-map games, no errors |
| 3 | Queen farming and persistent secondary champion | Complete | 59 cases / 382 assertions, release + sanitizers; 16 Python tests; refined native 6–2, zero errors |
| 4 | Purposeful portal routes | Complete | 61 cases / 395 assertions, release + sanitizers; native 5–3; sandbox 1–1, no errors, peak 56,860,251 |
| 5 | Paid-step economics and queen interception | Economics complete; interception next | 62 cases / 400 assertions, release + sanitizers; native 5–3, zero errors |

Every stage needs focused regression coverage, release and sanitizer checks, paired native comparisons, and saved source snapshots. Combined promotion requires metered games, multiple seeds, both colours and deterministic repeat checks. Native matches do not certify CPU budgets. No upload is part of this task.

## Stage 1 — queen corridors

Helpers score every retained segment against visible queen escape lanes and one-step continuations. Sole exits receive the strongest reservation; unsafe helper moves still lose to safe moves. Queens send advisory future landing intentions on odd rounds, expiring after one round. New children derive reservations from visible queen heads without needing inherited memory or delivered messages. Reports never certify remote occupancy. Fixed ID checks apply to both colours. `no-corridors` is a single-feature ablation.

Frozen snapshot: `build/leader-stage1-corridors`. Native seeds 601/602, both colours on current Slithery Fight / Default / Tower Defense / Portals: 11–5, zero errors (`build/validation/leader-stage1-native16`). Both sides retained six queens; summed final queen length was 57 versus 20. Allied fatal queen targets were seven versus eight, so blockage is reduced only modestly in this sample. Opening Slithery Fight traps remain. This is encouraging paired evidence, not an established competition advantage. The reusable `util/strategy_metrics.py` verifies reconstructed final engine results and records early population, scoring bodies, portal usage and paid steps.

## Stage 2 — territorial expansion

Investment capacity now scales with map area (eight to 32, bounded by the engine limit), so maps starting above eight units can invest. Complete long helpers may spawn a minimum-size child; early champions of at least eight segments can spare two, while smaller established champions remain protected. Independent body-validated income routes extend to four moves and can include directly observed spawners due within twelve rounds. Parent and child still need multiple six-turn escape routes, room and safe heads. Investment is refused when it freezes a visible queen corridor. `no-territory` restores the earlier growth rules for ablations.

Snapshot: `build/leader-stage2-territory`. Native seed 603, current Autarky / Islands / Around UNSW / Slithery Fight, both colours, versus stage 1: 5–3, zero errors (`build/validation/leader-stage2-native8`). Functional coverage and the focused future-income/cap tests pass; more population is not assumed to mean better play. The combined validation will measure early population and scoring bodies on broader seeds.

## Stage 3 — protected food routes and secondary champion

Protected units route toward remembered countdowns with travel and waiting costs; overdue predictions expire rather than becoming imaginary pearls. Fresh future food is exempt from the generic recent-visit penalty. Queens emit food-route claims alongside corridor/status reporting. Helper champions broadcast bounded self-length estimates using the existing heartbeat wire type; projected split/movement changes are included where simulation is possible. Reports remain advisory and do not change remote occupancy. The queen is excluded from the helper champion election, but a non-solitary helper must reach eight segments before receiving champion protection. Smaller units continue their ordinary collection/scouting roles.

A draft without the minimum champion size scored 4–4 on seed 604 and risked over-protecting every isolated small helper. It was refined before commit. Final snapshot `build/leader-stage3b-farms`: seed 605, current Portals / Maze / Tower Defense / Default, both colours, versus stage 2: 6–2, zero errors (`build/validation/leader-stage3b-native8`). Release and ASan/UBSan pass; the fresh/expired report and farm-countdown regressions pass. The new ablation profiles are checked to alter exactly one configuration flag. `no-farms` disables this stage.

## Stage 4 — portal goals and destination surveys

Designated small scouts seek unresolved portal boundaries and can probe after three stalled rounds, retaining the twelve-round cooldown, complete-body requirement, enemy-contact deferral and nearby-food preference. Helpers send surveys of currently visible, unoccupied landing tiles with multiple open continuations and nearby observed income. Tiles their planned movement will occupy are not advertised. Survey work is bounded to four candidates and 32 search nodes per candidate, with rotating coverage. Sonar beam axes rotate across four-round reporting cycles so each report type can reach both axes.

Starved protected units can proactively use a matching mapped pair and a survey no more than one round old. Queens above eight segments, champions above twelve, sole survivors and late-game units do not take this exploratory relocation. This remains explicitly uncertain: other units may occupy a reported empty exit later. Reports do not set seen/occupancy flags, and ordinary safe-move validation continues to reject unseen landings. Focused tests cover expiration, mapping, body conflicts and a queen choosing the surveyed exit after twelve rounds without income. `no-portal-routes` is the ablation.

Frozen snapshot: `build/leader-stage4-portals`, identical to all 32 selected working files. Native seed 606, current Portals / Maze / Stripes / Autarky, both colours, versus stage 3b: 5–3, zero errors (`leader-stage4-native8`). Summed final queen lengths 93 versus 32; four versus three queens survive. Sandbox seed 607, Around UNSW / both colours: 1–1, zero errors, candidate peak 56,860,251 / 100,000,000 (`leader-stage4-sandbox2`). These are small diagnostics; combined broader validation remains required.

## Stage 5a — paid movement economics

Helpers now preserve growth when a paid sprint merely collects enough pearls to replace its movement cost. Free extra steps, positive net growth, validated safety improvements and the separate funded enemy-queen trade remain available. `no-economics` disables this restriction for helpers.

Snapshot `build/leader-stage5a-economics`; seed 608, current Schooltime / Slithery Fight / Default / Stripes, both colours versus stage 4: 5–3, zero errors (`leader-stage5a-native8`). Candidate paid steps were 570 across 79,867 turns, versus 1,166 across 56,418 opponent turns: 7.14 versus 20.67 per thousand turns. Queen survival was three versus four, so reduced spending alone is not evidence of better queen protection. Release and ASan/UBSan pass 62 cases / 400 assertions.

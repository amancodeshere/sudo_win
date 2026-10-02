# Leader-driven upgrades

Baseline: `92667ed`, frozen v5 in `build/submission-competition-v5`.
Evidence: `LEADER_GAMEPLAY_REVIEW.md`. Current 17 map hashes are in the associated analysis JSON.

| Priority | Work | State | Evidence |
| --- | --- | --- | --- |
| 1 | Queen corridors and split-child yielding | Complete | 57 C++ cases / 369 assertions, release + ASan/UBSan; 16 Python tests; native 11–5 against v5 over 16 games, no errors |
| 2 | Earlier territorial expansion | Complete | Release + ASan/UBSan pass 57 cases / 373 assertions; native 5–3 over eight current-map games, no errors |
| 3 | Queen farming and persistent secondary champion | Pending | Measure scoring bodies, not total food |
| 4 | Purposeful portal routes | Pending | Test continuation safety and destination benefit |
| 5 | Paid-step economics and queen interception | Pending | Keep free movement; meter combined runtime |

Every stage needs focused regression coverage, release and sanitizer checks, paired native comparisons, and saved source snapshots. Combined promotion requires metered games, multiple seeds, both colours and deterministic repeat checks. Native matches do not certify CPU budgets. No upload is part of this task.

## Stage 1 — queen corridors

Helpers score every retained segment against visible queen escape lanes and one-step continuations. Sole exits receive the strongest reservation; unsafe helper moves still lose to safe moves. Queens send advisory future landing intentions on odd rounds, expiring after one round. New children derive reservations from visible queen heads without needing inherited memory or delivered messages. Reports never certify remote occupancy. Fixed ID checks apply to both colours. `no-corridors` is a single-feature ablation.

Frozen snapshot: `build/leader-stage1-corridors`. Native seeds 601/602, both colours on current Slithery Fight / Default / Tower Defense / Portals: 11–5, zero errors (`build/validation/leader-stage1-native16`). Both sides retained six queens; summed final queen length was 57 versus 20. Allied fatal queen targets were seven versus eight, so blockage is reduced only modestly in this sample. Opening Slithery Fight traps remain. This is encouraging paired evidence, not an established competition advantage. The reusable `util/strategy_metrics.py` verifies reconstructed final engine results and records early population, scoring bodies, portal usage and paid steps.

## Stage 2 — territorial expansion

Investment capacity now scales with map area (eight to 32, bounded by the engine limit), so maps starting above eight units can invest. Complete long helpers may spawn a minimum-size child; early champions of at least eight segments can spare two, while smaller established champions remain protected. Independent body-validated income routes extend to four moves and can include directly observed spawners due within twelve rounds. Parent and child still need multiple six-turn escape routes, room and safe heads. Investment is refused when it freezes a visible queen corridor. `no-territory` restores the earlier growth rules for ablations.

Snapshot: `build/leader-stage2-territory`. Native seed 603, current Autarky / Islands / Around UNSW / Slithery Fight, both colours, versus stage 1: 5–3, zero errors (`build/validation/leader-stage2-native8`). Functional coverage and the focused future-income/cap tests pass; more population is not assumed to mean better play. The combined validation will measure early population and scoring bodies on broader seeds.

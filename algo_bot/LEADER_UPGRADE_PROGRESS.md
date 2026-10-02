# Leader-driven upgrades

Baseline: `92667ed`, frozen v5 in `build/submission-competition-v5`.
Evidence: `LEADER_GAMEPLAY_REVIEW.md`. Current 17 map hashes are in the associated analysis JSON.

| Priority | Work | State | Evidence |
| --- | --- | --- | --- |
| 1 | Queen corridors and split-child yielding | Complete | 57 C++ cases / 369 assertions, release + ASan/UBSan; 16 Python tests; native 11–5 against v5 over 16 games, no errors |
| 2 | Earlier territorial expansion | Pending | Preserve priority-one reservations |
| 3 | Queen farming and persistent secondary champion | Pending | Measure scoring bodies, not total food |
| 4 | Purposeful portal routes | Pending | Test continuation safety and destination benefit |
| 5 | Paid-step economics and queen interception | Pending | Keep free movement; meter combined runtime |

Every stage needs focused regression coverage, release and sanitizer checks, paired native comparisons, and saved source snapshots. Combined promotion requires metered games, multiple seeds, both colours and deterministic repeat checks. Native matches do not certify CPU budgets. No upload is part of this task.

## Stage 1 — queen corridors

Helpers score every retained segment against visible queen escape lanes and one-step continuations. Sole exits receive the strongest reservation; unsafe helper moves still lose to safe moves. Queens send advisory future landing intentions on odd rounds, expiring after one round. New children derive reservations from visible queen heads without needing inherited memory or delivered messages. Reports never certify remote occupancy. Fixed ID checks apply to both colours. `no-corridors` is a single-feature ablation.

Frozen snapshot: `build/leader-stage1-corridors`. Native seeds 601/602, both colours on current Slithery Fight / Default / Tower Defense / Portals: 11–5, zero errors (`build/validation/leader-stage1-native16`). Both sides retained six queens; summed final queen length was 57 versus 20. Allied fatal queen targets were seven versus eight, so blockage is reduced only modestly in this sample. Opening Slithery Fight traps remain. This is encouraging paired evidence, not an established competition advantage. The reusable `util/strategy_metrics.py` verifies reconstructed final engine results and records early population, scoring bodies, portal usage and paid steps.

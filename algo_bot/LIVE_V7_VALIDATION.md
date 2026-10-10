# Replay upgrade validation — 2 October 2026

The five requested strategy priorities are implemented and committed separately, with three integration/correctness refinements. Functional and runtime checks pass. Competitive results are mixed: **this version has not demonstrated a reliable advantage over uploaded v4 or competition leaders**. The package is suitable for controlled evaluation; automatic promotion is not justified by these results. No upload or live challenge was performed during this work.

## Final source and package

- Strategy source commit: `805c418` (the final documentation/test commit does not alter submission sources).
- Selected sources: `build/submission-live-v7-finalc`, stable profile, 32 files.
- ZIP: `build/submission-live-v7-finalc.zip`.
- Source SHA-256: `32f0c87bded85193e57567bc437dc1a3e7b155eb0142362c652c666b75b6ee7a`.
- Archive SHA-256: `a7b8a93367d32e1b048a6a0ddd88728a7c34745e8d2c4634de237615fdc45141`.
- SDK runtime fingerprint: `61500032444809f70cc6f7046efd69f4786fb45c3476b74a2e6711aab696b826` (a different source-list/hash procedure from the packaging manifest).

All selected source bytes match the working tree. ZIP integrity and exact file membership were verified, with credentials, tests and utilities excluded. An independently regenerated archive has the identical hash. Earlier `submission-live-v7-final` and `submission-live-v7-finalb` packages are superseded; use **finalc** for this strategy.

The uploaded comparison baseline is submission 14744, `bot bot v4`, frozen at `build/submission-competition-v6-final`, original strategy commit `d5f44c7`. Comparison opponents are our frozen versions or the explicit local stress variant below, **not private leader executables**.

## Implemented behavior

1. Candidate-specific enemy responses defend queens and scoring dragons through their next action, accounting for retained necks, departing tails and stationary split children. Visible funding and uncertain enemy lengths are distinguished within fixed search budgets.
2. Growth checks complete bodies, six-turn corridors and independent income. Food-rich areas receive a larger early population budget; split rejection diagnostics expose why expansion stops.
3. Productive portal approaches value destination food and space. Expendable helpers keep exploration; protected units keep viable local farm routes. Stale surveys may motivate an approach but cannot certify an unseen landing.
4. Scoring coordination values real route progress and fresh larger champions, while preserving small helpers and checking queen rescue over a longer horizon.
5. Paid steps receive a segment cost unless they provide a validated escape or queen release. Collected food is remembered even when the action spends more segments than it earns.
6. Helpers prefer keeping a visible queen exit open when their own safety classification is equal. Validated paid movement may release jointly blocked queen exits.
7. The response horizon includes enemies with lower IDs: they act next round before our next action. The official-engine verifier now checks this cyclic ordering explicitly.

## Correctness and runtime

- **79 C++ cases / 555 assertions** pass in release on Apple Clang 21, LLVM Clang 22.1.8 and GCC 15.2.
- ASan/UBSan pass the same C++ cases.
- **16 Python tests** pass, including isolated policy profiles, packaging/replay tools and engine conformance.
- Pinned `unswbc` 1.2.5 engine SHA-256: `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`.
- Engine probes confirm queen-first / longest / total scoring, frozen starting-length free-step allowance, dead-queen zero and cyclic unit turns.
- Exact final-source metered peak: **53,610,292 / 100,000,000**, below the benchmark's 90% guard. Native games do not certify CPU usage.
- Final-source validation has zero recorded candidate or opponent runtime/action errors. Intentional helper head trades are allowed only after the benchmark independently validates them.
- Four repeat pairs (Portals and Schooltime, both colours) produce identical replay hashes. Eight files represent four unique fixtures.

Three observed live queen attacks and two multi-turn portal neck traps are retained as regressions. The neck-trap fixtures omit sonar and verify occupancy across movement/growth/splitting; they do not reconstruct a full-policy counterfactual win or invent an escape for an already trapped minimum-length unit.

## Final comparisons

Outcomes below are wins–losses–draws from the candidate's perspective. Every set uses both colours. The all-map set covers all 17 current maps; smaller sets are separate seeds and cannot be pooled into a representative competition win rate.

| Exact final-source set | Opponent | Fixtures | Result |
| --- | --- | ---: | --- |
| Native, all 17 maps, seed 812 | Uploaded v4 | 34 | **17–16–1** |
| Metered, Autarky / Slithery Fight / Schooltime / Around UNSW, seed 820 | Uploaded v4 | 8 | **2–6–0** |
| Native, Portals / Maze / Schooltime / Slithery Fight, seed 821 | Queen-exit predecessor | 8 | **4–4–0** |
| Native determinism, Portals / Schooltime, seed 816 | Uploaded v4 | 4 unique, repeated twice | **2–2–0** |

The metered subset is adverse evidence, not a certification of improved winning performance. Its different seed and map composition also mean the outcome difference cannot be attributed to metering alone.

### Broader head-trade stress comparison

The stress opponent is the exact frozen uploaded v4 with **only** `enable_favourable_trades` changed to `true`. It is a local constructed opponent, not a leader. Slithery Fight / Schooltime / Default / Around UNSW, seed 822, both colours:

| Candidate | Opponent | Fixtures | Result |
| --- | --- | ---: | --- |
| Exact finalc | Aggressive v4 variant | 8 | **1–7–0** |
| Original uploaded v4 | Same aggressive variant | 8 | **3–5–0** |

Both sets pass the action/runtime gates with zero errors on either side. This is adverse competitive evidence against promoting the combined strategy: response checks alone have not solved retaining large scoring units under broader helper attacks. These tests do not isolate which policy caused the regression.

## What the replays establish

Across the all-map comparison the final policy collects **16,008 vs 9,744 pearls**, makes **712 vs 441 portal crossings**, and has median round-100 population **6.5 vs 5**. Queens survive **11 vs 10** fixtures, with summed final queen lengths **194 vs 71**. However, summed final longest length is **602 vs 607**. Additional income and units are not consistently becoming retained scoring length.

The metered subset shows the same weakness more clearly: **8,437 vs 4,640 pearls** and median round-100 population **21.5 vs 13.5**, but summed final longest length **191 vs 227** and only two wins. There are **431 vs 274** children dying within five turns; these are raw counts alongside higher spawning, not normalized mortality rates. Large-dragon retention and crowding deserve further work before claiming a competitive upgrade.

In the all-map set our queen death classifications include nine self collisions, nine head collisions and five allied obstructions. Those outcomes support further attention to long-horizon farm geometry, traffic and adversarial defense; they do not prove a particular untested fix will win. The bounded response search cannot model unseen enemies or all future joint movement.

## Earlier experiments and limitations

The first combined draft scored **14–19–1** across 17 maps and **1–7** in its metered subset. Its portal ablation scored **3–5**, despite higher food and crossing counts. Productive destination requirements and keeping viable queen farms improved the balanced draft to **16–17–1** and **5–3** on its respective sets; its fresh-seed portal ablation was **5–3**. Those older-source results are retained but do not validate finalc bytes.

The queen-exit predecessor scored **15–18–1** in its all-map set and **4–2** in its six-game metered set. Correcting lower-ID response timing required fresh exact-source runs; these earlier results are also superseded. The older balanced Autarky peak of 64,516,858 belongs to that older source and is not the final-source peak.

`LIVE_V7_PROGRESS.md` records each stage, including losing comparisons, the corrected response timing and the original benchmark warning for an independently verified helper trade. `analysis/live_v7_validation_2026-10-02.json` stores compact outcomes, replay hashes, source identities, rules, map hashes and available strategy metrics. Raw matches and replay data remain in `build/validation/`.

The work establishes implemented features, tested contracts, reproducibility and runtime headroom. It does **not** establish that the bot is the best in the competition. Live performance against leader versions remains a separate evaluation.

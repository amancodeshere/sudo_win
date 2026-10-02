# bot bot v5 implementation and validation

Requested release: **bot bot v5**, description **please actually win**.
Baseline: active submission 14928, immutable `build/submission-live-v7-finalc`.
Findings: `LIVE_BOT4_REPLAY_REVIEW.md`. All comparisons are local, not private leader executables.

## Stage 1 — bounded response defense

Use a reverse distance lower bound on directed visible topology to prioritize and prune response routes, keeping all first-step checks. Record plausible unfinished branches as uncertainty; protected movement and queen investment/rescue cannot interpret budget exhaustion as clear. Budgets remain 160 shared/64 per enemy. Added exact visible topology/body regression for Around UNSW 880180 round 138: detect the funded four-step attack previously missed at this budget.

Validation/results are recorded below as each stage completes. Experimental policies must earn inclusion through both-colour benchmarks and judge CPU checks. No claim of guaranteed wins against private leaders.

Stage 1 release checks: 80 C++ cases / 560 assertions pass, including the recorded funded attack and deliberately exhausted-search uncertainty. Frozen verified source `build/submission-v5-defense-verified`, SHA `960886d5243f3ab192363ebed5997ea6560a03612ca2f8efc1eb0a59da9e811d`. Paired native and metered comparisons underway; no budget increase.

## Stage 2 — phase-aware productive expansion

A rich region is measured against nearby workers, not only global population. Through round 139, uncrowded income can fund a two-segment worker from a length 6–19 early champion even when the team already has four units. Parent/child body reconstruction, escape horizon, separate reachable resources, queen corridor and stationary threat gates remain required. Length-20+ champions and the consolidation phase keep their reserve. Fresh competing helper claims also remove shared funding opportunities; own claims do not disqualify investment. The `no-phase-expansion` profile isolates this policy.

Release C++ checks pass (including rich multi-unit champion investment, crowding rejection and phase cutoff); Python utilities pass. Snapshot `build/submission-v5-expansion`; paired three-map stage comparison seed 833 is running.

## Stage 3 — useful four-direction sonar network

Every scheduled turn carries four authenticated beams. Enemy helper heads as well as queens receive immediate warning payloads alongside claims/heartbeat; protected units consume these advisory warnings. One fresh ally report can be relayed per turn with original sender/time and per-observation loop suppression. Local disproval blocks pearl/head relays. Echo enemy-head counts change warning dissemination only: aggregate echoes never supply coordinates or free-space evidence. Existing queen corridor intent and resource/portal reports remain scheduled. `no-sonar-network` allows ablation.

Release C++ passes 81 cases / 575 assertions (wire provenance, relay age/loop bounds, all-head warnings, echo limitations and local disproval). The Python suite passes all 16 tests in the pinned SDK environment. An initial invocation using system Python lacked SDK modules and was rerun successfully in that environment; this was an environment mismatch, not a passing validation. Frozen `build/submission-v5-sonar`; isolated three-map paired comparison seed 834 running.

Stage 1 native comparison (early defense snapshot, equivalent production budgets): 6–2 vs uploaded finalc, seed 831, four maps, both colours, no action errors. The metered fresh seed is currently 0–2 on Around UNSW, peak 65,875,228; full result will be retained. Stage 2 isolated native first Schooltime pair is 0–2; no acceptance based on partial scores.

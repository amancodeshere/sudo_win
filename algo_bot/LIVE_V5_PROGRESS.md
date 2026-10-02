# bot bot v5 implementation and validation

Requested release: **bot bot v5**, description **please actually win**.
Baseline: active submission 14928, immutable `build/submission-live-v7-finalc`.
Findings: `LIVE_BOT4_REPLAY_REVIEW.md`. All comparisons are local, not private leader executables.

## Stage 1 — bounded response defense

Use a reverse distance lower bound on directed visible topology to prioritize and prune response routes, keeping all first-step checks. Record plausible unfinished branches as uncertainty; protected movement and queen investment/rescue cannot interpret budget exhaustion as clear. Budgets remain 160 shared/64 per enemy. Added exact visible topology/body regression for Around UNSW 880180 round 138: detect the funded four-step attack previously missed at this budget.

Validation/results are recorded below as each stage completes. Experimental policies must earn inclusion through both-colour benchmarks and judge CPU checks. No claim of guaranteed wins against private leaders.

Stage 1 release checks: 80 C++ cases / 560 assertions pass, including the recorded funded attack and deliberately exhausted-search uncertainty. Frozen verified source `build/submission-v5-defense-verified`, SHA `960886d5243f3ab192363ebed5997ea6560a03612ca2f8efc1eb0a59da9e811d`. Paired native and metered comparisons underway; no budget increase.

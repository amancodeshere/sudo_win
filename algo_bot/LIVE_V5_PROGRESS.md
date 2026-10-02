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

Release C++ passes 81 cases / 574 assertions (wire provenance, relay age/loop bounds, all-head warnings, echo limitations and local disproval). The Python suite passes all 16 tests in the pinned SDK environment. An initial invocation using system Python lacked SDK modules and was rerun successfully in that environment; this was an environment mismatch, not a passing validation. Frozen `build/submission-v5-sonar`; isolated three-map paired comparison seed 834 running.

Stage 1 native comparison (early defense snapshot, equivalent production budgets): 6–2 vs uploaded finalc, seed 831, four maps, both colours, no action errors. The metered fresh seed is currently 0–2 on Around UNSW, peak 65,875,228; full result will be retained. Stage 2 isolated native first Schooltime pair is 0–2; no acceptance based on partial scores.

## Stage 4 — earlier retreat and shared portal constraints

Among equal current safety classes, protected units prefer a continuation outside the currently observed funded attack envelope. This is an explicit heuristic for avoiding earlier encirclement, not certification of future enemy positions. Queen traffic viability still precedes the continuation preference. Portal helpers share fresh negative observations for constrained/threatened landings; income routes, surveyed crossings and blind helper probes honor these warnings. Negative reports expire after four rounds and newer productive observations supersede them. Unknown onward space alone cannot produce a trap warning. Release tests include pressure on a sole continuation and negative-survey age/precedence.

Source verification corrected the previous replay audit: existing portal safety already consulted enemy-head echoes; the earlier statement of no strategy use was too broad. The v5 scheduler adds separate echo-informed dissemination.

Completed isolated evidence: defense native 6–2; defense metered 1–3, zero errors, peak 65,875,228. Expansion 2–4 with unchanged queen survival (2/6 both sides), population medians (11 / 13.5 at rounds 25 / 100), lower total pearls (4,804 vs 5,412) and retained longest sum (114 vs 141); this policy has not earned final inclusion. Sonar 3–3 native, zero errors. Full combined acceptance and ablations remain required.

## Stage 5 — controlled feeding prototype and recovery instrumentation

Implemented an opt-in (`feeding`) worker-to-queen transfer policy. It requires a starved nonqueen/nonchampion length-4–12 worker, at least three allies, a current-round report confirming the queen's fully visible body, no nearby observed/reported enemy, no immediately available natural queen meal, at least half the donor length recoverable as new pearls within three ordinary moves, and a six-step recipient escape horizon. Paid movement is not used for the predicted pickup. The default remains disabled until representative recovery and competitive results justify it.

Retirement uses the supported default action plus a recognized `INDICATOR SUDO_WIN_DONATION` diagnostic, never an invented engine opcode. Independent benchmark checks reject missing actions without this explicit marker and reject donation markers without complete donor bodies and reachable predicted food. Replay metrics now count verified donations and actual queen recovery within four rounds. Pinned-engine conformance demonstrates alternating donor segments becoming pearls and the fixed queen actually growing from two to four.

The emission integration test caught a fall-through that would have emitted fallback MOVE after the retirement indicator; corrected before acceptance, with a regression asserting indicator-only output. The early `v5-feeding-experiment` artifact/seed-837 run is superseded and must not be uploaded or used as acceptance evidence. Corrected frozen pair: `v5-feeding-verified` and `v5-feeding-verified-control`, seed 838. C++ release and 17 Python tests pass.

Stage 4 isolated native comparison: 3–3, no errors. A separate twelve-map both-colour comparison versus the upload, seed 836, finishes 12–12 with no errors. More activity is not being presented as competitive proof.

## Stage 6 — scoring-aware optional movement cost

Optional paid movement now prices queen segments at three times the worker shadow cost and champion segments at twice that cost. Immediate net income remains valued; validated safety improvements and queen corridor releases retain their exemption. Existing regressions still require a paid escape when it is necessary, reject zero-net paid collection tempo and preserve free movement toward income. All new profile flags have packaging-isolation checks, including the opt-in feeding variant.

Release C++ remains 84 cases / 603 assertions; 17 Python tests pass. Frozen combined candidate `build/submission-v5-priced`, plus `v5-no-phase`. Fresh paired pricing comparison seed 840 and phase-ablation seed 841 are running. Judge runs check the integrated defense/network/territory policy against the upload; the feeding policy is not enabled for acceptance by default.

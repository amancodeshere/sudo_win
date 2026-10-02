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

## Integration correctness review

Separated certified response funding from the invented partial-length envelope: invented free-step allowance can no longer fund an attack using observed length alone. A length-four enemy without income cannot certify a four-step sprint; one intermediate pearl can fund it. Added this regression alongside the recorded length-five attack. Response searches now skip topology work entirely when no enemy head is visible and order enemies by directed attack distance, including portals. Sonar field bounds are checked before encoding all scheduled beams, so a helper identity outside the wire range cannot turn a good planned action into an exception fallback.

The stage comparisons above predate this funding refinement; final exact-source native and metered comparisons are required before upload.

## Release selection and exact candidate

The combined phase-ablation test (seed 841, Schooltime/Slithery Fight/Around UNSW, both colours) favors the conservative expansion limits **5–1**. Together with the initial expansion regression and unchanged early population, the new `phase-expansion` policy remains opt-in. The release still uses productive independent-resource splitting, queen corridors and mature-champion retention. The feeding experiment (corrected seed 838) is **3–3** and triggers zero qualifying donations; engine recovery works in conformance fixtures, but match-level benefit is not established, so feeding remains disabled. Optional scorer pricing is **3–3** in its isolated six games with no errors.

The pre-funding-refinement integrated metered comparison (seed 839, Portals/Slithery Fight) finishes **1–3**, zero errors, peak **49,591,995**. It is preserved as adverse evidence, not overwritten by final results.

Exact release candidate: `build/submission-v5-release-candidate.zip`, source SHA **f5f4128d70d0f99323520f76cd10792f35aef5a5b5d1286d2ea762221642a823**, ZIP SHA **047e26470c38f9615228f2d55c178ae89296f554926b7431ac39cd7c4ecb7610**. A separately regenerated ZIP matches both hashes; archive membership and all 32 source file bytes match the repository, with credentials/tests/utilities excluded. Final 17-map seed-842 comparison, metered seed-843 comparison and aggressive-reference seed-844 stress test are running against this exact artifact.

## Release-gate regression and refinement

The first candidate passes judge correctness (8 metered games **4–4**, peak **67,456,789**, zero errors), all compiler/sanitizer checks (86 cases / 609 assertions), 17 Python tests, and four replay repeat pairs. However, the broad comparison is losing too many games, and the aggressive comparison is only **3–5**; this candidate is not accepted for upload yet.

Two new tests fail on that source: a queen takes tempting food at an endpoint with a modeled four-step possible attack even though an unthreatened west move exists; and a helper takes food in a fresh remote queen-intent tile. Fixed by keeping every modeled response within the five-step envelope below the clear safety class, including stationary queen split/rescue checks, and by raising authenticated fresh queen intent to corridor-release priority. The original attack funding remains certified independently; a possible route is not relabeled as certainly funded. Expired intent still releases the helper's food. These changes require new exact-source comparisons; the old release candidate is superseded.

## Resource-cap ablation correction

Further source review found that disabling phase expansion still retained its local crowding condition in the resource population cap. This unintentionally restricted the old rich-region capacity even in the conservative release and makes the preceding no-phase selection an incomplete rollback. Corrected the off branch to use the original `local_income >= 4` cap gate; experimental crowding remains confined to `phase-expansion`. A regression exercises a rich region above the geometric population budget with a nearby helper. Earlier candidate comparisons remain preserved, but new acceptance artifacts must contain this correction. Minimal defense/network configuration comparisons are being used to assess which added policies help.

## Corrected-cap configuration selection

On fresh seed 850, the complete corrected-cap policy beats the reduced defense/network policy **4–2** (Default, Schooltime, Around UNSW, both colours). Retain the complete policy, with experimental expansion and voluntary feeding disabled. The reduced policy's broader and metered results are still recorded; no claim of superiority is drawn from a partial series.

Pre-cap candidates finish **12–22** and **14–20** in their respective 17-map native comparisons. The first candidate scores 3–5 against the aggressive reference, while the uploaded baseline scores 5–3 on exactly that same seed/maps/colours; the complete pre-cap policy is rejected. It did demonstrate sonar delivery improvement: 144,594 other-ally hits / 115,986 personal turns (~1,247 per 1,000) versus baseline 42,721 / 124,590 (~343 per 1,000) in its metered comparison. Better delivery is not equated with better final scoring.

The selected artifact `build/submission-v5-cap-corrected` contains the resource-cap fix and all passing queen response/intent regressions. Final exact-source 17-map seed 853, metered seed 854, and same-seed aggressive stress comparisons are running. The final upload script will be repinned to this artifact, not an older candidate.

Final immutable upload package: `build/submission-v5-final.zip`, source SHA **d797f5adb34dda4af29da68f1102a6d4e1933dbc59cde2fc163fd9126cd68ace**, ZIP SHA **4e7a2e88bba4b7f2ca50f814b710f33738d6f6b57dd2b645d999abecb84195b9**. These match the tested corrected-cap snapshot and a fresh independent regeneration. All 32 selected files match current repository bytes and exact archive membership. Upload name/description are pinned in a credential-safe script, which checks the ZIP hash before POST. API access and active baseline 14928 are verified. Upload awaits completion of exact-source judge/comparison gates.

## Final validation — selected corrected-cap source

The exact frozen upload completes all 17 maps, seed 853, both colours: **14–20**, with zero candidate/opponent action errors. Metered four-map seed 854 completes **4–4**, zero errors, peak **75,778,850 / 100,000,000** instructions. Same-seed aggressive-reference comparison completes **4–4**; the prior upload achieved **5–3** on those same eight fixtures. These are mixed/adverse competitive results and do **not** demonstrate superiority over the uploaded baseline or private leaders. The complete configuration is retained after beating the reduced corrected-cap network configuration 4–2 directly; that reduced variant's separate broad 17–17 and metered 2–6 results are preserved as well.

Selected broad replay metrics: queens survive **12/34 vs 14/34**, final queen-length sum **124 vs 174**, longest sum **482 vs 613**, pearls **13,557 vs 14,064**; ally collision still accounts for 11 of the candidate queen deaths versus six for the baseline. Metered sonar delivers **152,729** other-ally hits in **125,795** personal turns (~1,214 per 1,000) versus **39,005 / 120,043** (~325 per 1,000). More useful deliveries have not yet translated into better final scoring. Queen coordination, feeding and longer-term scoring remain the main unresolved weaknesses.

Final source passes **88 C++ cases / 619 assertions** under Apple Clang 21, LLVM Clang 22.1.8, GCC 15.2 and ASan/UBSan, plus **17 Python tests** in the pinned SDK environment. Four unique deterministic fixtures (seed 855, Devil/Autarky, both colours) each repeat with identical replay bytes: eight executions, unique outcomes 1–3. All 32 packaged source files and ZIP members match the repository and tested snapshot, and independent regeneration reproduces both hashes. Experimental phase expansion and voluntary feeding stay disabled.

Compact immutable comparison evidence, source fingerprints, per-match replay hashes, pinned rules and all rejected-stage outcomes are saved in `analysis/live_v5_validation_2026-10-03.json`. Verification is complete; upload proceeds under the user's explicit request with exact name **bot bot v5** and description **please actually win**. Live challenge matches have not been queued.

# Leader-driven v6 validation

The priority-ordered changes are implemented and separately committed. The final candidate passes runtime, CPU and reproducibility gates, with a modest internal match advantage. It is **not established as the best competition bot**. No upload or activation was performed.

## Exact candidate

- Directory: `build/submission-competition-v6-final`
- Upload archive: `build/submission-competition-v6-final.zip`
- Source/flag manifest: `build/submission-competition-v6-final.json`
- Selected-source SHA-256: `c971c782f0341a8ef3eade03d87684ccb668177b9b58193755dab5ab8f94a839`
- ZIP SHA-256: `0e8b1b36aae52f6f60b7259bb929bb89ae34d49f213b2ac7ba6dfd30b2582957`

All 32 selected working files match the package byte-for-byte. A second independent package produced identical source and ZIP hashes. Tests, replay fixtures, documentation, utility scripts and credentials are excluded from the upload sources.

The map set is the **17-map snapshot downloaded on 2026-10-02**, with hashes retained in `analysis/leader_v6_validation_2026-10-02.json`. This does not assert that server maps cannot change later. Judge/toolkit: `unswbc==1.2.5`; verified engine SHA-256 `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`. Conformance checks pass for fixed queen scoring, free movement allowances, starting-length sprint pricing and dead-queen scoring.

## Changes and commits

| Commit | Change |
| --- | --- |
| `39c10db` | Helpers reserve queen exits and receive short-lived queen movement intentions |
| `5a6fa55` | Population investment scales with map size and independently reachable future food |
| `507214e` | Protected countdown farming and a separate, bounded helper champion election |
| `d540cae` | Scouts seek portal boundaries; protected relocation uses fresh destination surveys |
| `4a7883f` | Helpers reject paid zero/negative growth movement without a validated reason |
| `daef2c1` | Small hunters/blockers spread over legal enemy-queen interception routes |
| `d973d0b` | Split children use validated free movement when their bodies obstruct queen exits |
| `b087a4b` | Small helpers can pay to provably release a visible queen's sole exit safely |
| `022674e` | Protect four-segment secondary scorers after round 80; enforce actual-length eligibility |

Queen reservations, food/portal reports and interception sightings remain advisory. They never certify unseen occupancy. Candidate safety and survival ranking remain above ordinary incentives. The sole-exit exception requires body-validated removal and the full six-turn survival horizon. Larger growing units, sole survivors and fixed queens remain excluded from small-helper sacrifices. Portal relocation is deliberately uncertain and bounded by size, starvation, cooldown, mapping and a fresh survey; larger protected bodies retain the ordinary policy.

`LEADER_UPGRADE_PROGRESS.md` preserves the component checks and intermediate variants. The immediate four-segment champion prototype was refined after it hurt early expansion. Its results, and the earlier v6 freeze, must not be attributed to the final candidate.

## Functional and judge checks

- **68 C++ cases / 438 assertions**, passing LLVM Clang release, Apple Clang release, GCC 15 release, and ASan/UBSan debug builds.
- **16 Python tests**, including isolated feature profiles, submission contents, reproducible packaging, replay auditing and judge-rule checks.
- Replay-derived regressions cover split-child free movement and the paid move needed to release a queen exit. Additional cases cover stale reports, distinct interception goals, walls, unknown cells, free/paid economics and champion eligibility.
- Every strategy report reconstructs final dragon counts, total lengths and longest bodies and checks them against the engine result.

## Exact-source match results

| Suite | Opponent / maps / seeds | Results | Candidate peak points |
| --- | --- | --- | --- |
| `leader-v6-final-sandbox34` | v5; all 17 maps; seed 701; both colours | **20–14**, zero errors | **56,902,887** |
| `leader-v6-final-sandbox16` | v5; Around UNSW / Slithery Fight / Portals / Maze; seeds 702/703; both colours | **8–8**, zero errors | **51,985,836** |
| `leader-v6-final-native34-bot3` | Uploaded bot3 sources; all 17 maps; seed 704; both colours | **18–16**, zero errors | Unmetered |
| `leader-v6-final-determinism8` | Self-play; Devil / Slithery Fight; seed 710; both colours; two repetitions | Eight games; four pairs of identical replay hashes; zero errors | **50,351,870** |

The exact final source therefore completed **58 metered games and 34 native diagnostic games**. Competitive metered results against v5 total **28–22 (56%)**. Self-play colour wins are not an improvement measure. Native comparisons do not certify CPU budgets. All metered candidate turns stayed below the benchmark's 90-million margin and the engine's 100-million limit.

The final champion refinement separately scored **9–7** against the paid-release stage on the four focused maps, seeds 707/708, both colours (`leader-late-champion-native16`). Those additional native games are component diagnostics, separate from the table above.

## What improved and what remains weak

Across the 50 metered v5 comparisons:

| Measurement | Final candidate | v5 |
| --- | --- | --- |
| Queens surviving | 16 | 16 |
| Summed final queen length | 123 | 120 |
| Summed final longest body | 904 | 834 |
| Pearls collected | 25,883 | 19,032 |
| Portal crossings | 847 | 464 |
| Declared paid steps per 1,000 personal turns | 5.18 | 10.76 |
| Fatal queen targets occupied by allies | 14 | 19 |

These are aggregate measurements, not scoring formulas. Longer survival and more units create more personal turns. Increased food and portal totals do not automatically mean better primary queen scores. Overall queen survival against v5 has not improved in this sample. In the separate bot3 comparison, allied queen blockages remain particularly frequent (15 versus seven), despite more candidate queens surviving overall (11 versus eight).

The Slithery Fight opening trap is concretely improved: the frozen predecessor loses both queens at round one in the focused opening comparison, while the free-tail-clearance variant keeps them alive until rounds 110/223. Later queen deaths still occur.

The final candidate lost **all six Portals games**, both Queen of Spades games, and both Tower Defense games against v5. Portals queens remain small even when retained; the next useful work is reliable access to productive farms/portal entrances and stronger prediction of allied body blockage before entering narrow routes. The small-helper paid release does not solve obstruction by longer helpers. On Maze the final candidate scored 5–1 across the three seeds; Around UNSW and Slithery Fight each scored 4–2.

The first v6 freeze scored 27–23 over the corresponding all-map native plus focused metered v5 samples and 23–11 against bot3. The final variant's 28–22 against v5 and 18–16 against bot3 show a **tradeoff**, rather than a universal improvement over every opponent. Private leader source and metered head-to-head results against the leaders are unavailable here. The map/seed sample is small and unevenly weighted; no leaderboard rank or competition win is claimed.

## Saved evidence and continuation

The compact, tracked evidence is `analysis/leader_v6_validation_2026-10-02.json`. Full JSONL results, source fingerprints, death snapshots, metrics and replays remain under `build/validation/leader-v6-final-*`. The feature history is saved in `LEADER_UPGRADE_PROGRESS.md`. Upload only the exact candidate directory or archive named above when choosing this variant; earlier similarly named packages are preserved diagnostic variants.

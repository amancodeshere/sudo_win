# New bot replay assessment — submission 14928

The uploaded **bot bot 4**, description **i shall win**, server version **6**, won **3 of 51** newly requested unranked games and lost 48. The immediate failures are queen survival, insufficient early territory, poor conversion of growth into retained scoring bodies, and a communication policy that uses sonar much less effectively than the observed opponents. The implementation is legal and within the instruction limit; strategic performance is the problem.

This report separates direct replay measurements, known behavior in our source, reconstructed-state diagnostics, and proposals requiring validation. No algorithm change, upload, or live challenge was performed during this assessment.

## Scope and verification

The authenticated API snapshot was captured **2026-10-02 12:38:17 UTC / 22:38:17 Sydney**. Battle history contained 98 series and covered the submission's upload time; it was not truncated at the 200-series limit. Downloaded 56 recent own-team replays into `replays/`. Exactly 51 replay headers identify **14928**; five ranked AlgoMaster games identify **14744** and are excluded from every new-bot statistic.

| New-bot opponent | Games | Wins–losses | Profile rank observed |
| --- | ---: | ---: | ---: |
| tungtung67 | 17 | 0–17 | 18 |
| EternalWisdom | 17 | 3–14 | 77 |
| Adrak vali chai | 17 | 0–17 | 13 |

These are stronger opponents, but not the current top three. Separately downloaded **15 recent leader-versus-leader games**: horse against Cutlery, SSS and Sponge, five predefined maps per pairing (Schooltime, Portals, Slithery Fight, Autarky, Around UNSW). The initial pending Sponge/Around UNSW game was retrieved once completed. The top-three snapshot order was Cutlery, SSS, Sponge. Team profile reads occurred slightly later than the leaderboard snapshot; ranks and ratings can move.

All **66 included replay reconstructions** agree with the engine's final dragon count, total length, longest body and winner. Winners are reconstructed using queen length, then longest dragon, then total length at the round limit, with elimination handled separately. All use replay format 2. Replay/map hashes and identities are preserved. All 51 own challenge games put us on team A; these are not both-colour paired-seed experiments. Leader games have different seeds/opposition and are descriptive comparisons, not causal performance tests.

Our largest recorded instruction count was **61,997,211 / 100,000,000**, with **zero reported timeouts**. Enemy instruction counts are redacted, so their compute costs cannot be compared.

## What decided the 48 losses

- **27:** our entire team eliminated.
- **10:** round limit, inferior queen length.
- **11:** round limit, queen lengths tied and inferior longest dragon.
- **0:** total length required to decide a loss.

Our queen survived **5/51**, versus **21/51** opponent queens. Of 46 own queen deaths, **30 were head-to-head**, eight had a fatal allied-body target, five hit their own body, and three hit another enemy body. Fatal target classification does not establish that an earlier escape existed.

Median queen death was round **160.5**, at length **two**. **33/46** dead queens were length two. We made 36 queen splits; 29 retained a length-two parent. Twenty-one of the 33 length-two queen deaths followed a last queen split that retained two segments. Some small queens had no such split, so rescue does not explain every small-queen death.

In **20/30** fatal head attacks the attacker was visible in the queen's preceding observation. The killing step was first in 23 attacks, second in four, third in one, and fourth in two. The central weakness is not merely a horizon that is one step too short: small enemies can kill an encircled queen with one step.

## 1. Queen defense needs earlier escape and honest search uncertainty

The new response model is a real improvement in representation: it checks resulting occupancy, vacated cells, split children, and all visible enemies through our next action. However, it can run out of search before testing a dangerous route, and static six-step self-survival is not six rounds of adversarial survival.

**Concrete search-budget weakness:** [Around UNSW 880180](https://game.battlecode.au/battles/880180), round 138. A visible length-five enemy kills the queen on its fourth move step. On the reconstructed queen history, the normal 160 shared continuation nodes / 64 per enemy classify the recorded endpoint as no funded or possible response. Changing only those diagnostic budgets to 4,096 / 1,024 finds a **funded four-step attack**. The larger-budget diagnostic was local and unmetered, not an uploaded fix or a proven winning alternative.

The source returns zero response steps after an exhausted search without an explicit completeness flag (`src/combat/combat.cpp`, response search). Therefore a truncated search can look like a cleared endpoint. The next repair should expose exhaustion, prioritize plausible shortest attack routes, and keep uncertainty when coverage is incomplete. A blanket 25-fold budget increase needs metered testing before use.

**Concrete late entrapment:** [Default 880207](https://game.battlecode.au/battles/880207), round 198. Our length-two queen moves south and a visible length-two enemy kills it with one east step. The source diagnostic identifies the funded attack. The apparent east alternative has no continuation in the same diagnostic; this does not establish an easy winning move on the final turn. The bot needs to avoid entering the encircled position earlier, while it still has space or length to escape.

**Concrete rescue limit:** [Australia 880181](https://game.battlecode.au/battles/880181), round 155. The length-nine queen has no ordinary empty first step, splits seven segments into a child, retains two, and is killed by an adjacent length-two enemy immediately afterward. The source diagnostic labels the split `forced queen loss retains secondary contender`. That fallback is intentionally a last resort, not a certified safe queen rescue. The child's survival cannot replace the fixed queen's scoring role.

Replayed **11,052 queen observations** with reconstructed inboxes, echoes and recorded actions maintaining the world model. The diagnostic agrees with all queen actions in 14 games, and with **45/46 fatal actions**, but differs on 1,085 historical actions overall. Native/compiler behavior, reconstructed body directions and planner-state divergence mean this is **not a bit-exact recovery of every server-side state**. In these diagnostics 17 of the 30 fatal head endpoints are classified funded, two uncertain within three steps, two uncertain at four steps, and nine undetected. Those counts concern the reconstructed model, not a claim that the online bot knowingly chose all those threats.

Recommended next work: incomplete-search risk; enemy-aware continuation safety; earlier retreat based on shrinking exits and local enemy density; uncertainty near the vision boundary and unseen portal approaches; and danger reports for **enemy helpers and champions**, not only the enemy queen.

## 2. We still fail to establish enough productive territory

Both sides below are measured on the same continuing matches at each checkpoint.

| Median end-of-round state | Our bot | Direct opponents | Games |
| --- | ---: | ---: | ---: |
| Round 25 units | 4 | 7 | 51 |
| Round 25 total length | 13 | 17 | 51 |
| Round 25 pearls collected | 3 | 8 | 51 |
| Round 100 units | 4 | 21 | 49 |
| Round 100 total length | 16 | 50 | 49 |
| Round 100 pearls collected | 15 | 86 | 49 |
| Round 100 longest dragon | 8 | 4 | 49 |
| Round 200 units | 4 | 43 | 41 |
| Round 200 total length | 20 | 107 | 41 |
| Round 200 longest dragon | 10 | 5 | 41 |

We retain relatively large early bodies while the opposition builds a broad population of small dragons. Their aggregate food is **46,817 vs 9,390**, over approximately four times as many personal turns. Total food combines natural spawns and death-generated pearls; the provenance analysis below separates them.

Against horse on the selected five-map samples, round-100 population medians are **36 for Cutlery, 56 for SSS, 19 for Sponge and 32 for horse**. These samples differ from our direct matches, but establish that substantial early expansion occurs under strong opposition.

Our source now permits a larger resource-funded population cap, so simply raising that cap again is unlikely to resolve populations stuck near four. Relevant constraints remain: early champion protection at length eight, champion investment only below four units while retaining eight, independent nearby income, full-body/corridor checks, and a split score that must exceed movement. These source constraints are confirmed; replays alone do not identify which guard rejected every split.

Recommended next work: measure rejection reasons on faithful restored unit histories; use an explicit early territory-building phase followed by consolidation; allocate independent regions and spawn timing; and reserve stable queen space while other dragons expand. Expansion should be judged by population, income, queen survival and retained score together.

## 3. How the other teams use sonar

Each beam carries a 64-bit payload, stops at the first body or kelp, may traverse portals, and can reach either team. The reverse-facing beam leaves the tail; inbox delivery occurs on the recipient's next turn. Echoes describe what rays hit and arrive on the sender's next turn. These are official engine features, not a global team broadcast. [Official sonar rules](https://game.battlecode.au/docs/sonar).

Replay rays expose emission timing, recipient, hit kind and payload. They **do not expose the opponent's decoder, interpretation or decision logic**. A bot sending sonar does not prove that it uses echoes, forwards messages or makes a particular choice because of them.

| Team / sample | Games | Beams per personal turn | Other-ally deliveries per 1,000 turns | Other-ally hits / beams |
| --- | ---: | ---: | ---: | ---: |
| Our bot | 51 | 1.12 | 245 | 21.8% |
| tungtung67 | 17 | 3.12 | 1096 | 35.1% |
| EternalWisdom | 17 | 2.46 | 667 | 27.1% |
| Adrak vali chai | 17 | 3.94 | 1268 | 32.2% |
| Cutlery | 5 | 2.41 | 914 | 37.9% |
| SSS | 5 | 3.22 | 1222 | 37.9% |
| Sponge(Albert and Bob) | 5 | 1.82 | 591 | 32.5% |
| horse | 15 | 2.94 | 906 | 30.8% |

“Other ally” excludes self-hits. Deliveries include duplicate payloads sent in different directions; they are not unique discoveries. Larger populations and different board positions improve contact opportunity, so higher delivery rates cannot be attributed solely to superior beam aiming.

Observed transmission styles:

- **Our bot:** exactly two beams on 92,668 turns and none on 72,115 turns. Both beams carry the same report. Only **56.2%** of turns emit a report; **65.8%** of beams stop on kelp. We delivered 40,339 beams to other allies and 1,345 directly to our queen.
- **tungtung67:** predominantly three beams, with four on 47,689 turns; sends on about **97.9%** of turns. Nearly every active turn repeats one payload across its beams. This is broad repeated coverage, not proof of a particular message meaning.
- **Adrak vali chai:** four beams on 192,238 turns, roughly **98.4%** of turns. About **28.5%** of sending turns use multiple distinct payloads across beams.
- **EternalWisdom:** four beams when transmitting, about **61.5%** of turns; approximately **22.8%** of sending turns have multiple payloads.
- **Cutlery / Sponge / horse:** often different payloads on different beams. Horse uses multiple payloads on virtually every sending turn; Cutlery and Sponge do so on most sending turns. **SSS** uses four beams on most active turns in this sample, with both repeated and distinct payload patterns.

The leaders are visibly using more of the available communication bandwidth and, in several cases, multiplexing different packet values within one action. The private packet semantics remain unknown; it would be unjustified to label their fields as food, reservations or map data without stronger evidence.

What is specifically missing in our implementation:

1. `src/bot/bot.cpp` sends two rotating beams, not four, and shares one phase-selected payload across them.
2. Our helper supports protocol 3 echoes, but the strategy never calls `get_sonar_echoes()`. Enemy/body contacts outside the vision window therefore do not become risk information in our world model.
3. Enemy-head reports are currently generated only for IDs 0 and 1. Other enemy heads are omitted even though helper attacks dominate many losses.
4. Report scheduling is round-phase based, so urgent danger or a new farm may compete with routine pearl/portal reports. There is no delivery-aware resend or explicit forwarding of received reports.
5. Remote reports expire after eight rounds and the report store holds 64 sender/type entries. Durable resource-region ownership and a consistently shared scorer identity need more than intermittent local heartbeats.

Our own codec is functioning: **all 185,336 outgoing packets validate**, with matching encoded sender IDs. Reconstructed reads accept **46,724** own-codec packets, including 39,400 from other allies and 7,324 self-deliveries. Of those reads, 21,641 are pearl messages, 13,309 feeder claims, 3,693 portal reports, 3,079 surveys, 2,532 helper heartbeats, 1,518 danger reports, 772 queen champion reports and only 180 enemy-queen sightings. No consumed enemy-origin packet passes our own team's validation in the reconstructed reads. Two foreign outgoing packets happen to match our foreign-team codec tag; this is insufficient evidence of shared protocol or copied code.

The inbox reconstruction applies the engine's first-turn legacy filtering before our helper negotiates protocol 3. Replay files do not record foreign protocol negotiation, so foreign “consumed” inbox counters mean packets available to a protocol-3 recipient, not proven use by the opponent.

Recommended next work: four bounded beams with important messages selected by urgency; destination/ally-aware coverage; fresh head-danger reports for any enemy; cautious use of delayed aggregate echoes; relay of important reports with original timestamp and deduplication; and stable scorer/resource claims. An echo cannot certify a particular unseen cell as empty, and sonar cast after a fatal move cannot save that move.

## 4. Portal activity needs to become income and retained score

We cross **520 portals**, versus **6,100** for the direct opponents: **3.16 vs 9.33 per 1,000 personal turns**. This is still a material normalized gap, not just a population-count effect. Our queen crosses four times versus their 52, but the evidence supports productive helper/collector territory first rather than blind queen relocation.

On the three Portals games:

| Opponent | Our pearls | Their pearls | Our final longest | Their final longest | Outcome |
| --- | ---: | ---: | ---: | ---: | --- |
| tungtung67 | 310 | 1,132 | 9 | 48 | Loss, queens dead |
| EternalWisdom | 7 | 1,262 | 8 | 35 | Win, our queen 3 vs theirs 0 |
| Adrak vali chai | 333 | 1,691 | 14 | 50 | Loss, queens dead |

The portal changes can establish more income than the former two-pearl games, but not consistently. The EternalWisdom win is a valid queen-first scoring win and **does not establish successful portal farming**.

[Portals 880211](https://game.battlecode.au/battles/880211): at round 365 the queen splits from six to two plus a four-segment child; at 366 it has no ordinary empty first step and dies against an allied body. [Portals 880187](https://game.battlecode.au/battles/880187): at round 437 a length-two queen has no ordinary empty first step and dies entering its own body. These are prevention/continuation failures, not evidence of an available legal final-step escape.

Recommended next work: a shared registry of productive portal regions, remembered trap/exhaustion information, multiple independent collector destinations, and a landing plan that accounts for neck occupancy and subsequent body growth. Compare expected income against a protected dragon's existing farm before relocating it.

## 5. The best observed strategies expand first and retain scorers later

All three Schooltime games leave both queens at length three. Our longest dragons finish **23, 4 and 15**, versus **88, 20 and 64** respectively. These are direct demonstrations that fallback scoring remains weak even when queen safety is not the immediate decider.

Across the 24 games in which our team survives, median final longest is **15.5 vs 49**. Across all 51 games it is **0 vs 21**, with our 27 eliminations included. We lost 107 nonqueen dragons that had reached at least 12 segments: 41 other-body deaths, 35 self collisions and 31 head attacks. These counts use peak length, so the unit may have split or spent segments before death; they are not 107 deaths at length 12 or higher.

The current top-team sample retains median final longest lengths **51 (Cutlery), 64 (SSS), 41 (Sponge)**, while their round-100 medians were only **5, 3 and 4**. This supports a phase transition from broad small-unit territory to retained late scorers. It does not reveal their exact phase thresholds or private algorithms.

A scorer alone cannot replace queen survival: [Autarky 880085](https://game.battlecode.au/battles/880085) ends with our longest 49 and total length 105 against their total 106, but our queen is dead and theirs is 61. The queen tier decides that loss before total length.

Recommended next work: preserve reliable income and escape space for the fixed queen and one or a few mature scorers, while workers expand elsewhere; apply adversarial defense to scorers; coordinate helper right-of-way; and stop investing a mature scoring body when regional expansion has low marginal value. Avoid protecting every isolated length-eight dragon as if it were the globally selected champion.

## 5a. A major missing feature: concentrate worker resources through death-food recovery

The engine converts alternating segments of a dead body into pearls. I tracked the origin of every collected pearl through the replay's death and tile-change events, preserving existing pearl origins when a drop overlapped an existing pearl. Every origin count sums to the independently measured food total in all 66 games. [Official death sequence](https://game.battlecode.au/docs/execution-order).

Across our direct matches, opponents collect **26,268 natural/initial pearls**, **19,377 from allied deaths**, and **1,172 from our deaths**. We collect **5,619 natural/initial**, **2,708 from allied deaths**, and **1,063 from enemy deaths**. The natural resource gap remains substantial even after death-food is separated.

The most revealing result is how the strongest teams feed their queens:

| Team | Games | Queen food: natural/initial | Queen food: allied deaths | Queen food: enemy deaths | Allied share of queen food |
| --- | ---: | ---: | ---: | ---: | ---: |
| sudo win | 51 | 310 | 38 | 11 | 10.6% |
| Cutlery | 5 | 15 | 183 | 2 | 91.5% |
| SSS | 5 | 10 | 118 | 2 | 90.8% |
| Sponge(Albert and Bob) | 5 | 5 | 21 | 2 | 75.0% |
| horse | 15 | 39 | 355 | 1 | 89.9% |

These are matched by collection event and drop provenance, not inferred from total food alone. SSS collected **110 of its 118 allied-death queen pearls within four rounds** of the donor's death; horse collected **265 of 355** within four rounds. On [SSS versus horse, Slithery Fight 880510](https://game.battlecode.au/battles/880510), SSS's queen eats 110 allied-death pearls and only five natural pearls, finishing at length 64. Of those allied-death pearls, 61 arrive within one round and 104 within four rounds. Thirty-one came from allies whose replay action was `suicide`.

This demonstrates effective conversion of worker-held length into queen food. **Deliberate coordinated feeding is a strong hypothesis, not a proven reading of private code.** A replay suicide action may be explicit retirement or a default action after missing output; wall deaths are also not inherently proof of intent. What is certain is that the resulting food is harvested quickly and overwhelmingly supports their queens.

Our strategy has food claims, collectors, splits and rescue children, but **no planned donor lifecycle or score-aware body-to-food transfer policy**. Its growth check expects independent child income, rather than valuing a child whose purpose is to feed a protected scorer. This is an important missing capability to prototype, alongside productive expansion, rather than treating every friendly death as pure waste.

A useful experiment would coordinate surplus workers near a safely accessible scorer, predict available drop cells, and retire only when expected recovered score exceeds the donor's future income and tactical value. It must account for enemy theft, conversion loss, movement timing, queen exits and actual pickup access. Do not copy indiscriminate suicide or wall moves: compare a targeted transfer policy with ordinary collection and retain it only if queen/scorer survival and final scoring improve.


## 6. Movement economics remain a secondary gap

We declare **827 paid steps**, versus opponents' 920, but over much fewer turns: **5.02 vs 1.41 per 1,000 turns**. Our queens declare 59 paid steps versus their two. These are requested costs; a collision can prevent later requested steps from executing. A paid escape can still be correct.

EternalWisdom is particularly economical in this sample: about **0.245 paid steps per 1,000 turns**, against our 3.49 in those same games. This accompanies much larger food and population, so the important comparison is sustained income and retention, not an unconditional prohibition on sprinting.

Recommended next work: measure segment cost per surviving scorer and per successful income relocation; avoid equal-yield paid detours; preserve free movement and proven queen escapes. CPU micro-optimization is not the first priority while the live sample has no timeouts.

## Suggested implementation order and acceptance criteria

1. **Repair response-search uncertainty and prevent earlier encirclement.** Regress Around UNSW 880180; expose exhaustion; strengthen protected continuation risk without assuming unseen occupancy is known.
2. **Make early expansion region-based and phase-aware.** Instrument actual split rejections; broaden sustainable worker territory while keeping queen exits and reserved income.
3. **Improve sonar coverage and usefulness.** Add all-head danger, cautious echo use, urgent message scheduling, relays and stable resource/scorer claims. Measure other-ally deliveries and actual downstream use, not just sends.
4. **Coordinate queen rescue, helper traffic and retained farms.** Regress forced queen rescues and portal traps, act before a minimum-length parent is already trapped, and preserve late scoring bodies.
5. **Add score-aware feeding and consolidate portal territory.** Prototype controlled body-to-food transfer with actual pickup access and donor value; require evidence of successful income regions and retained scoring length. The queen-food provenance makes this a substantial opportunity, not merely a movement-cost tweak.
6. **Tune paid movement with live retention evidence.** Preserve necessary escapes and demand durable benefit from optional spending.

Every proposed policy needs isolated ablations, fresh seeds and both colours against the frozen uploaded bot and aggressive local opponent, followed by metered checks and controlled live leader comparisons. Preserve the actual attack and trap observations as regressions. Reject a policy that only increases pearls, portals or splits while worsening queen survival or retained scoring length. The present evidence does not establish that every loss is preventable or that a particular proposed change will beat every leader.

## Per-map new-bot outcomes

Each cell links its replay and shows our/their queen length (Q) and longest body (L). W/L is the engine outcome, with fixed queen scoring and elimination honored.

| Map | tungtung67 | EternalWisdom | Adrak vali chai |
| --- | --- | --- | --- |
| Around UNSW | [L: Q 0:6, L 16:64](https://game.battlecode.au/battles/880180) | [W: Q 0:0, L 29:27](https://game.battlecode.au/battles/880083) | [L: Q 0:0, L 12:50](https://game.battlecode.au/battles/880204) |
| Australia | [L: Q 0:6, L 38:36](https://game.battlecode.au/battles/880181) | [L: Q 0:22, L 21:41](https://game.battlecode.au/battles/880084) | [L: Q 0:0, L 0:41](https://game.battlecode.au/battles/880205) |
| Autarky | [L: Q 0:6, L 4:110](https://game.battlecode.au/battles/880182) | [L: Q 0:61, L 49:61](https://game.battlecode.au/battles/880085) | [L: Q 0:0, L 0:5](https://game.battlecode.au/battles/880206) |
| Default | [L: Q 0:6, L 0:32](https://game.battlecode.au/battles/880183) | [L: Q 0:29, L 21:39](https://game.battlecode.au/battles/880086) | [L: Q 0:0, L 0:6](https://game.battlecode.au/battles/880207) |
| Devil | [L: Q 0:0, L 0:4](https://game.battlecode.au/battles/880184) | [L: Q 0:0, L 0:4](https://game.battlecode.au/battles/880087) | [L: Q 0:0, L 0:4](https://game.battlecode.au/battles/880208) |
| Islands | [L: Q 0:0, L 19:50](https://game.battlecode.au/battles/880185) | [L: Q 0:0, L 40:43](https://game.battlecode.au/battles/880088) | [L: Q 0:0, L 0:21](https://game.battlecode.au/battles/880209) |
| Maze | [L: Q 0:7, L 2:63](https://game.battlecode.au/battles/880186) | [L: Q 0:0, L 11:35](https://game.battlecode.au/battles/880089) | [L: Q 0:0, L 0:28](https://game.battlecode.au/battles/880210) |
| Portals | [L: Q 0:0, L 9:48](https://game.battlecode.au/battles/880187) | [W: Q 3:0, L 8:35](https://game.battlecode.au/battles/880090) | [L: Q 0:0, L 14:50](https://game.battlecode.au/battles/880211) |
| Prisoners Dilemma | [L: Q 0:0, L 0:3](https://game.battlecode.au/battles/880188) | [L: Q 0:3, L 0:3](https://game.battlecode.au/battles/880091) | [L: Q 0:0, L 0:3](https://game.battlecode.au/battles/880212) |
| Queen Of Spades | [L: Q 0:5, L 0:5](https://game.battlecode.au/battles/880189) | [L: Q 0:0, L 0:9](https://game.battlecode.au/battles/880092) | [L: Q 0:0, L 0:3](https://game.battlecode.au/battles/880213) |
| Schooltime | [L: Q 3:3, L 23:88](https://game.battlecode.au/battles/880190) | [L: Q 3:3, L 4:20](https://game.battlecode.au/battles/880093) | [L: Q 3:3, L 15:64](https://game.battlecode.au/battles/880214) |
| Slithery Fight | [L: Q 0:7, L 7:59](https://game.battlecode.au/battles/880191) | [L: Q 0:0, L 21:79](https://game.battlecode.au/battles/880094) | [L: Q 0:0, L 12:67](https://game.battlecode.au/battles/880215) |
| Stripes | [L: Q 0:0, L 0:3](https://game.battlecode.au/battles/880192) | [L: Q 0:0, L 0:5](https://game.battlecode.au/battles/880095) | [L: Q 0:3, L 0:3](https://game.battlecode.au/battles/880216) |
| Tower Defense | [L: Q 0:0, L 0:3](https://game.battlecode.au/battles/880193) | [L: Q 0:0, L 0:7](https://game.battlecode.au/battles/880096) | [L: Q 0:3, L 0:3](https://game.battlecode.au/battles/880217) |
| Trauma | [L: Q 0:2, L 8:34](https://game.battlecode.au/battles/880194) | [W: Q 29:0, L 29:21](https://game.battlecode.au/battles/880097) | [L: Q 0:13, L 23:23](https://game.battlecode.au/battles/880218) |
| Trophy | [L: Q 0:6, L 0:6](https://game.battlecode.au/battles/880195) | [L: Q 0:2, L 0:4](https://game.battlecode.au/battles/880098) | [L: Q 0:0, L 0:4](https://game.battlecode.au/battles/880219) |
| weakhold | [L: Q 0:0, L 0:3](https://game.battlecode.au/battles/880196) | [L: Q 0:16, L 0:16](https://game.battlecode.au/battles/880099) | [L: Q 0:0, L 0:4](https://game.battlecode.au/battles/880220) |

## Evidence and reproducibility

- Compact tracked data: `analysis/live_bot4_replay_review_2026-10-02.json`.
- Raw own replays: `replays/<game>.replay`; index `replays/bot-bot-4-submission14928-study-2026-10-02.json`.
- API captures, leader replays, full audits, exact queen inbox/echo histories, actual attack paths and native diagnostics: `build/validation/live-bot4-submission14928-study/`.
- Investigation script copies are preserved under that directory's `investigation/`; existing tracked `util/analyze_replays.py` and `util/strategy_metrics.py` independently verify final states and movement/resource counts.
- Selected uploaded sources remain identical to finalc source SHA-256 `32f0c87bded85193e57567bc437dc1a3e7b155eb0142362c652c666b75b6ee7a`, ZIP SHA-256 `a7b8a93367d32e1b048a6a0ddd88728a7c34745e8d2c4634de237615fdc45141`.

Official references checked during the investigation: [sonar](https://game.battlecode.au/docs/sonar), [splitting](https://game.battlecode.au/docs/splitting), [execution order](https://game.battlecode.au/docs/execution-order), [game format](https://game.battlecode.au/docs/game-format). Opponent decision logic remains private. Measured behavior, source audit and diagnostic experiments are kept distinct from inferred intent.

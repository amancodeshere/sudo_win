# Leader gameplay review — 2 October 2026

Our largest remaining gap is coordinated strategy: clearing the queen's routes, establishing food territory early, and concentrating enough growth in the dragon that determines the result. Increasing portal attempts or population alone can make those problems worse.

This is an assessment of the current v5 candidate, not a claim that its competition rating has changed. The uploaded bot remains bot bot 3. No strategy code or competition submission changed during this review.

## Evidence and scope

The authenticated leaderboard snapshot was captured at **2026-10-02 05:18:27 UTC / 15:18:27 Sydney**. The first five teams were:

| Rank | Team | Elo | Games analysed |
| --- | --- | ---: | ---: |
| 1 | [Sponge(Albert and Bob)](https://game.battlecode.au/teams/213) | 2052 | 25 |
| 2 | [SSS](https://game.battlecode.au/teams/91) | 2014 | 27 |
| 3 | [forgot to mention](https://game.battlecode.au/teams/264) | 1995 | 10 |
| 4 | [horse](https://game.battlecode.au/teams/842) | 1994 | 44 |
| 5 | [Cutlery](https://game.battlecode.au/teams/306) | 1992 | 27 |

Source: [competition leaderboard](https://game.battlecode.au/leaderboard). Ratings move continuously; these are the study snapshot values.

Downloaded and reconstructed **79 unique completed games**: 54 games between these leaders and 25 games against the common opponent Deer Park, five per leader. A game between two leaders contributes two team observations, giving 133 observations. Series selected: 854853, 854858, 854863, 854873, 854634, 854700, 855256, 855261, 855326, 855331 and 855276. Public API read routes are documented in the [competition API documentation](https://game.battlecode.au/docs/api).

The event audit follows movement, splits, deaths, food, portals and fixed queen identities. Reconstructed final dragon count, total length and longest dragon match the engine result in **every one of the 79 games**. Two games were also reconstructed into board images and inspected visually. This is event-level gameplay analysis, not access to opponents' private source code.

For our current source, ran 34 native diagnostic games against the frozen bot bot 3 package: all 17 current maps, seed 501, both colours. Result: **18 wins, 16 losses, zero bot errors on either side**. Separately ran four metered sandbox games on the two current 64×64 maps, Around UNSW and Australia, seed 502, both colours: **2 wins, 2 losses, zero bot errors**, maximum candidate CPU count **65,000,985 / 100,000,000**. The native run provides no CPU certification; the four sandbox games do not certify every current map or seed.

Current candidate fingerprint: `f619db18e68cddbf3c84afc722220057b77cd3f3153dad072002baf65e106d33`. Engine revision: `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`; toolkit 1.2.5. Submission ZIP remains the existing v5 ZIP.

Machine-readable metrics, replay identities/hashes, current map hashes and the candidate queen collision audit are saved in [analysis/leader_gameplay_2026-10-02.json](analysis/leader_gameplay_2026-10-02.json). Full local diagnostics are in `build/validation/current-maps-native34/`, `build/validation/current-large-maps-sandbox4/` and `build/validation/leader-study/`. Raw external research downloads remain in `/tmp/sudo-win-leader-study/`.

## What the gameplay shows

| Bot / sample | Median population at round 100 | Median peak population | Pearls per 1,000 unit turns | Portal crossings per 1,000 unit turns | Declared paid steps per 1,000 unit turns |
| --- | ---: | ---: | ---: | ---: | ---: |
| Sponge | 18.5 | 39 | 66.6 | 9.05 | 2.08 |
| SSS | 24 | 49 | 63.4 | 6.44 | 6.86 |
| forgot to mention | 25 | 33.5 | 91.8 | 5.39 | 3.89 |
| horse | 24 | 50.5 | 73.8 | 7.50 | 2.61 |
| Cutlery | 9 | 17 | 86.7 | 11.73 | 4.35 |
| Our v5, current 34-game diagnostic | 4 | 27.5 | 55.5 | 1.31 | 10.86 |

These are descriptive comparisons with different opponents, seeds and map mixes. They identify hypotheses; they do not establish a head-to-head win rate. Round-100 medians exclude games ending before that round. Paid steps are requested steps beyond the action-start free allowance, not a measurement of segments actually lost before an interrupted move.

Population is not uniformly high among winners: Cutlery's median peak is below ours. The stronger shared pattern is developing useful units early and directing growth toward the winning objective.

### 1. Queen corridors and parent–child coordination — first priority

In our 34 current-map games, the queen survived seven times and died 27 times. At each fatal action, the independent visible checker found **no legal first step**. The fatal target belonged to an ally in **15 cases**, an enemy in six, and the queen itself in six. Twenty-six deaths occurred at length two; one at length three.

The ally collision classification describes the fatal target, not proof that every death was avoidable earlier. It does establish that queen safety is not solved by stronger enemy-head avoidance alone.

Concrete examples:

- Default, candidate A, round 406: queen 0 is surrounded by its own body, allied units 10 and 50, and a wall. Its mandatory west move hits unit 50. The queen eventually dies with no move available, despite our side collecting 229 pearls against 62.
- Slithery Fight, both colours: our queen dies at round one against allied unit 14 or 15 after the opening rescue split. Parent survival needs to account for what the child does next.
- Tower Defense, candidate A: 277 pearls against 26 still loses; both queens are dead and our longest dragon is six against twenty.

**Proposed change:** communicate queen movement intentions and reserve a short escape corridor. Helpers, including newly split children, should yield before they close that corridor. Add earlier dead-end rejection for the queen and turn-order-aware parent–child escape planning. Preserve legal-action checks and bounded search.

Touchpoints: [planner](src/planner/planner.cpp), [simulation](src/planner/simulation.cpp), [splitting](src/splitting/splitting.cpp), [roles](src/roles/roles.cpp), sonar/world reports.

Acceptance evidence: reduce allied queen blockages and improve surviving queen length on paired current-map seeds; include the two Slithery Fight openings and the Default round-406 state. A fallback that dies with no legal move is not by itself a first-step validation defect.

### 2. Early expansion into separate food territories

Leaders generally expand before local starvation. In [horse versus SSS on Autarky, game 854648](https://game.battlecode.au/battles/854648), SSS repeatedly splits its starting length-14 helper during rounds 0–5. horse also splits helpers while visible ordinary moves remain available. By round 25 they have 15 and 14 units. horse wins by eliminating the other team at round 276 while its own queen is only length two.

On Slithery Fight, the leaders' median round-25 populations range from 19 to 34; ours is ten. On Islands, theirs range from nine to sixteen; ours is five. These are map-name comparisons, not identical-seed contests.

Our investment policy refuses queens, helpers longer than twelve, and any team already at eight units. It additionally requires two six-step escape branches for each resulting unit and separate currently visible pearls within three steps. Several new maps start with eight or more units, so investment growth is disabled immediately. **Eight is an investment threshold, not a hard population cap:** rescue splits can still take our team to 64 later.

**Proposed change:** use a map- and phase-dependent investment budget, allow productive long helpers to spawn scouts, and estimate future food territory rather than demanding two immediate visible pearls. Keep parent/child viability checks and the queen corridor reservation from priority one. Give children distinct destinations through available communication rather than sending them into the same farm.

Measure round-25/100 productive population, unique territory reached, food per unit turn and child survival. Our child mortality within five personal turns is 15.8%, within the leaders' observed 13.3–21.9% range; simply demanding near-zero scout losses would miss the economic trade-off.

### 3. Convert food into queen growth or a durable secondary champion

The best counterexample to “more portals wins” is [horse versus Cutlery on Portals, game 854701](https://game.battlecode.au/battles/854701):

| Metric | horse | Cutlery |
| --- | ---: | ---: |
| Team pearls | 1,331 | 434 |
| Team portal crossings | 469 | 61 |
| Final queen length | 0 | 60 |

horse's queen crosses a portal at round 22 and dies at round 24. Cutlery's queen collects 57 pearls, never splits, and pays for no declared extra movement steps. Its only queen portal crossing occurs at round 497, so early queen portal relocation does **not** explain this win. Ordinary sustained queen farming does.

In [Maze game 854711](https://game.battlecode.au/battles/854711), Cutlery finishes with a length-119 queen after collecting 180 queen pearls, with four queen splits and no paid queen steps. In [Portals game 854853](https://game.battlecode.au/battles/854853), Sponge finishes with a length-28 queen despite twenty queen splits. Those examples show that a queen can sometimes invest successfully, but do not prove that relaxing our queen split restriction alone improves results.

Our current-map run collects 13,118 pearls against the old bot's 7,478. Nevertheless, all sixteen losses reach the round limit: three lose on queen length and thirteen lose on the subsequent longest-dragon comparison. One of those thirteen has equal surviving queens; twelve have both queens dead.

**Proposed change:** reserve recurring food territories for the queen while it is alive, using remembered spawn times and safe cyclic routes. Maintain a separate team-wide secondary champion with fresh self-reported length, even while a queen exists. When queen outcomes are lost or tied, protect that champion's growth and reduce splitting/payment that destroys the largest surviving body. Current champion reporting accepts queen senders only; remote helper election remains based on partial local sightings.

### 4. Portal expansion as a planned route

Our observed portal rate is 1.31 crossings per 1,000 unit turns, versus 5.39–11.73 among the leaders. The implementation explains part of this: probes are restricted to small non-champion helpers, generally wait eight rounds without resource progress, and impose a twelve-round cooldown. Queens consider remembered escape portals only after ordinary movement fails.

In Autarky game 854648, horse's queen crosses at round 85 while two ordinary visible escapes still exist, and returns through the known pair at round 89. It survives both crossings. This is an example of proactive relocation succeeding; the failed horse queen crossing in Portals game 854701 shows the opposing risk.

**Proposed change:** send designated expendable scouts toward portals earlier, share exit occupancy and nearby food/spawner evidence, and compare estimated destination income with the current territory. Route queens and champions through a mapped exit only when the benefit and continuation safety justify it. Avoid blindly transferring the leaders' portal volume to our queen policy.

An exit not previously seen by the crossing unit may still have been learned through sonar. Replay observations cannot prove an opponent took a blind gamble. Preserve this distinction when evaluating portal risk.

### 5. Spend movement length only for measurable value; improve useful offense

Our declared paid-step rate is higher than all five leaders' samples. Most of those payments are made by helpers; our queens request just 38 paid steps across the 34 games. Free multi-step movement remains valuable and should stay enabled.

**Proposed change:** record paid-step purpose and resulting net growth; require a credible income, escape or queen-kill benefit. Penalise neutral paid helper movement that repeatedly collects food without building a durable champion. Judge the policy by final scoring bodies and territory gained, not sprint count alone.

Enemy-queen pressure is also strategically relevant: **57 of the leaders' 100 queen deaths in this sample are head-to-head collisions**. Our hunting policy currently executes short, visible queen attacks by length-two/three helpers. Longer-term interception, blocking escape lanes and coordinated approach roles are reasonable next experiments. The replay statistics do not establish that any particular opponent uses a coordinated interception algorithm.

## Current-map validation gap

The live API supplies **17 active maps**. Seven were absent from our earlier ten-map validation: Stripes, Tower Defense, Islands, Around UNSW, weakhold, Maze and Australia. Six older maps changed: Schooltime, Default, Slithery Fight, Trophy, Prisoners Dilemma and Autarky. Four remain identical: Portals, Queen Of Spades, Devil and Trauma.

Earlier opening conclusions and win rates therefore cannot be applied to today's full pool without checking map versions. In particular, an old forced opening trap must not be assumed to remain forced on a changed map. The new 34-game diagnostic addresses functional coverage; stronger performance claims still require multiple seeds, both colours and metered tests across the current maps.

## Implementation order and decision gates

1. Queen corridor reservations and parent–child yielding.
2. Earlier territorial expansion under those reservations.
3. Queen farm routing and a persistent secondary champion.
4. Purposeful portal scouting and destination-based relocation.
5. Paid-step economics and coordinated queen interception.

Build and commit each strategy separately in the established `aman/feat: ...` style. For each stage, compare against frozen v5 and bot bot 3 on current map hashes, use multiple paired seeds and both colours, verify CPU headroom, and inspect queen/body outcomes rather than total food alone. Run ablations so any gain can be attributed to the individual change. Direct current-v5 competition matches against strong opponents remain outstanding.

The leaders also lose queens frequently: only 33 of 133 leader team observations finish with a living queen. Their private implementations and CPU budgets are unavailable; public replay instruction counts are zero. Copying every observed split, suicide or crossing would be unjustified. The concrete goal is earlier productive control while keeping a scoring body alive.

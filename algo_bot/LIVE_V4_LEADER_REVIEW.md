# Live v4 replay review — 2 October 2026

The first priority is defending the queen against later-moving enemies. The next priorities are earlier productive expansion, portal exploration that establishes food territory, and preserving a durable fallback champion. The uploaded bot's improvements against our old bots did not transfer to the strongest live opponents.

## Data and verification

Captured the authenticated competition API at **2026-10-02 09:16 UTC / 19:16 Sydney**. At that snapshot the top three were SSS (2102 Elo), Sponge(Albert and Bob) (2091), and forgot to mention (2028). Our team was rank 149, Elo 1263.

Downloaded and audited **81 completed games**: all 66 available games belonging to submission **14744, `bot bot v4`**, plus 15 recent games between Cache me outside, Sponge, horse, Delusion Tax and forgot to mention. These leader matches provide a comparison under strong opposition, rather than only against us. No additional challenges were created.

| Opponent group | Games | Our wins–losses |
|---|---:|---:|
| SSS, rank 1 | 17 | 0–17 |
| Sponge, rank 2 | 17 | 0–17 |
| forgot to mention, rank 3 | 17 | 1–16 |
| Other ranked opponents: Nexus Lab, Thermal Throttle, Proof by Intimidation | 15 | 10–5 |
| All v4 games in this snapshot | 66 | 11–55 |

All 51 direct top-three games place us on team A and cover the 17 maps once per opponent. They are unranked challenges; the other 15 are ranked. This is not a paired-seed, both-colour experiment, and the groups must not be combined into a claim about a general win probability. Opponents can also update their submissions.

Reconstructed movement, food, splits, deaths and portals. Final unit count, longest dragon and total length match the engine result in **all 81 games**. All 66 own replay headers identify submission 14744. Winner reconstruction agrees with fixed queen length → longest dragon → total length, with elimination handled separately, in all 81 games. Seven games distinguish this rule from longest-first scoring; the live winners follow queen-first scoring. Do not use the older cached public engine source as the live scoring specification.

Our largest instruction count was **51,773,306**, with **zero reported timeouts**. This sample points to strategic failures rather than instruction-limit failures. Opponent instruction counts are redacted.

Raw own games are saved in `replays/`; an index is `replays/bot-bot-v4-study-2026-10-02.json`. Leader replays, full audits, exported observations, API snapshots and investigation scripts are in `build/validation/live-v4-leader-study/`. Compact tracked evidence and replay hashes are in [analysis/live_v4_leader_review_2026-10-02.json](analysis/live_v4_leader_review_2026-10-02.json).

## What actually decided the leader losses

Of 50 losses, **34 ended with our team eliminated**, 11 reached the limit and lost on longest dragon, and five reached the limit and lost on queen length. None required total length to decide the winner.

Our queen survived **7/51** games versus the leaders' **25/51**. The median round of our 44 queen deaths was 118:

- **38 head-to-head attacks** by enemies.
- Three fatal targets occupied by allied bodies.
- Three fatal targets occupied by the queen's own body.

The last two categories describe the fatal target, not proof that an earlier escape was available. The prior local benchmark's emphasis on allied blockage is insufficient for the live leaders: enemy attacks dominate this sample.

## 1. Defend against enemy responses after our chosen action

In **31/38** fatal head attacks, the attacker was visible in the queen's preceding observation. The killing step was the first in 14 cases, second in 15, third in six, and fourth in three. Extending the horizon alone cannot explain or repair most of these failures.

Current threat prediction searches the board before our action. Our departing body can mask a route that becomes legal after our move or split. **Ten fatal enemy paths touched a queen-occupied cell in that earlier observation**, including three attacks immediately after a queen rescue split. Partial enemy bodies also make the policy too confident: 19 visible attackers exposed only two segments; their actual lengths can fund movement beyond that lower bound.

I ran the current C++ threat module on the exported fatal observations. In this deliberately limited diagnostic, only **13/38** attacked endpoints were classified as direct or funded later-enemy threats within three steps. These observations omit historical sonar and memory, so this is evidence about the visible threat calculation, not an exact reconstruction of the full bot's internal state or proof of 25 preventable deaths.

Concrete examples:

- [Stripes, game 864982](https://game.battlecode.au/battles/864982), round 98: our queen's north–west move puts its head on its previous body cell. A visible length-three enemy kills it with one east step. That target is blocked in the pre-action threat board.
- [Slithery Fight, game 865145](https://game.battlecode.au/battles/865145), round 168: a visible enemy showing two segments, actually length four, reaches the queen in three steps. The visible-only diagnostic marks the attacked endpoint as no threat.
- [Tower Defense, game 864983](https://game.battlecode.au/battles/864983), round 172: the queen splits and a visible adjacent enemy kills the stationary parent with one north step.

**Recommended change:** evaluate bounded enemy responses on each candidate's resulting occupancy, including vacated tails and rescue splits. Separate guaranteed attacks from uncertain attacks supported by partial bodies; the queen needs a stronger uncertainty penalty and earlier retreat. Incorporate enemy reach into continuation safety so a six-step path through otherwise empty space is not mistaken for a defensible escape. Preserve CPU bounds and test the actual attack routes as regressions.

## 2. Establish productive territory much earlier

| Median at end of round 100 | Our bot | Direct leader opponent |
|---|---:|---:|
| Living units | 3 | 28 |
| Total length | 15 | 66 |
| Pearls collected | 15 | 88.5 |

These medians use the same **46 games** still running at that point. At round 25, across all 51 games, the population medians were already 3 versus 8. In the separate leader-versus-leader sample, round-100 medians were 29 and 19 units across 13 continuing games: rapid expansion also occurs under strong opposition.

Our expansion policy still requires two six-step escape branches for both parent and child, independent income within four simulated steps, and an expansion score greater than the best movement score. It also protects early champions and queens. Those are source-level constraints; the replays do not reveal which guard rejected every opportunity.

**Recommended change:** measure split rejection reasons first. Replace the rigid two-escape rule with bounded, turn-order-aware viability in narrow productive corridors. Allocate scouts and collectors by reachable food regions and spawn timing, with separate destinations. Allow controlled early investment from a well-fed queen or helper when it preserves an escape and creates real income. Increase population budgets only where territory supports them; a larger cap cannot fix a team stuck at three units.

## 3. Make portals an income and expansion strategy

Across the 51 direct games, we crossed portals **192 times**, versus **5,958** for the leaders. Per 1,000 personal turns this is approximately **2.1 versus 10.7**, so the difference is not solely their larger populations. Queen crossings were much closer, **35 versus 42**: the strongest evidence supports more effective helper exploration first.

On the Portals map we collected **two pearls in each of all three games**, and our queen collected none. Against SSS and forgot to mention, both portal-exploring helpers died by round 41; the remaining queen and helper finished at length three each without collecting further food. The win against forgot to mention was earned because its queen died; it does not demonstrate successful portal territory control. Against Sponge our team collected two pearls despite repeated later rescue splits.

[Portals, game 864977](https://game.battlecode.au/battles/864977) shows helpers dying at rounds 25 and 41 while trying to cross into their own body through a mapped portal. Their observations had no ordinary empty first step. Exploration therefore needs continuation checks and escape planning, alongside greater ambition.

The protected-unit relocation code currently acts only when already beside a portal, starved for 12 rounds, small enough, and receiving a survey no more than one round old. Any remembered food/spawn route can suppress relocation. It does not explicitly choose and navigate toward the most promising surveyed entrance.

**Recommended change:** assign early scouts to different entrances, remember destination food income and hazards, avoid repeated exhausted loops, and route collectors toward successful regions. Compare destination income against current income before routing a queen or champion toward an entrance. Keep landing and onward-path checks; do not merely allow blind queen crossings or copy the leaders' crossing count.

## 4. Preserve a fallback champion and its farm

Eleven top-three losses were decided by longest dragon after queen lengths tied. On Schooltime, both queens finish at length three in all three games, but our longest bodies are **3, 3 and 5**, against **127, 59 and 111**.

In [Maze, game 865140](https://game.battlecode.au/battles/865140), our queen reached length 56. After rescue splitting, its length-two parent died against allied unit 68, whose body was length 56. That unit later died at length 57 in a fourth-step enemy attack; our final longest was nine versus 58. This illustrates both rescue coordination and the loss of an already-grown fallback body.

Strong opponents retain substantial bodies against each other: [horse versus Cache, Queen Of Spades, game 867859](https://game.battlecode.au/battles/867859) ends with horse's length-75 helper after both queens die. [Sponge versus Cache, Slithery Fight, game 867263](https://game.battlecode.au/battles/867263) ends with Sponge's queen at 64, with no paid queen steps. The game can reward either scoring route.

**Recommended change:** preserve a selected champion's income region and escape space while expansion continues elsewhere. Coordinate queen rescue children before they obstruct the parent, and apply the improved enemy-response defense to large champions. Track retained scoring length, not just food collected or peak length.

## 5. Refine movement economics after the larger failures

We declared **6.8 paid steps per 1,000 turns** against the leaders' **2.8**. Pearl income per 1,000 turns was **57.3 versus 74.5**. These are declared movement costs, not a claim that every requested paid step completed before a collision. Food counts here check that a pearl actually existed before removal.

**Recommended change:** log each payment's purpose and distinguish durable net growth, escape, portal access and queen attacks. Preserve free movement. A tactical escape can justify spending length; repeated payments without retained growth need a stronger penalty. Do not indiscriminately forbid paid movement, which can worsen queen defense.

## Validation for the next version

Implement each priority separately and retain the established `aman/feat: ...` commit style. First add the observed attack and portal traps as meaningful regressions. Run ablations against frozen v4 on current map hashes, multiple seeds and both colours, with metered judge checks. Track queen attacks/deaths, round-25/100 territory, champion survival and income by region. Then evaluate direct leader matches if requested; old-bot benchmark wins alone are insufficient acceptance evidence.

This review saves the replay evidence and proposed changes. The uploaded algorithm remains at commit `d5f44c7`; no new algorithm or submission was created during this assessment.

# Bot bot 3: replay assessment and proposed next steps

Submission 14545, server version 4. Snapshot: 67 battle series, 408 saved replays; 75 newly downloaded, 333 already present, zero unavailable or download errors. All 70 games with replay header identifying 14545 were audited event by event. The other five new files (848944–848948) belong to 14465 and were also audited separately in `build/live-v3-late-audit/`; they ended 1 win / 4 losses. No bot source, strategy flags, commits, activation, or live challenges were changed for this review.

Current-version record: **21 wins / 49 losses**. Losses: **29 team eliminations / 20 round-limit losses**. Final standings reconstructed from movement and split events match the replay results for both teams in every game. Queen-based ranking also independently matches every round-limit winner. No own timeout or no-valid-action deaths occurred; own peak CPU was 36,416,730 / 100,000,000 points.

## Critical rules mismatch

Current official rules differ from the cached local judge and parts of our strategy. The previous local win comparisons therefore do not establish performance under the current live rules. The earlier longest-snake scoring assumption was wrong for these games.

- Scoring: eliminate the enemy team, otherwise rank by fixed queen length, then longest living snake, then total living length. A dead queen has length zero. The queen is the starting lowest-ID snake on its team (ID 0 or 1); Team A is not universally ID 0. [Official scoring](https://game.battlecode.au/docs/structure).
- Sprinting: first `ceil(starting_length / 4)` steps are free; the starting length is fixed for the whole action. Only later steps cost segments. Our `Simulation::advance`, threat budget, and independent benchmark checker assume a payment for every step after the first. [Official movement](https://game.battlecode.au/docs/movement).
- The replay independently confirms 672 successful extra movement steps that cost our snakes zero segments. In game 849542, snake 78 at round 178, length 6, used WSS, ate two pearls, and hit its own segment on step 3. The free second step retained a segment our simulation removed. This is a concrete correctness failure, not simply an aggressive tactic losing.

## Recommendations in implementation order

1. **Synchronize the judge, simulator and checker with current rules.** Pin a current engine version; model free sprint allowance from action-start length everywhere, including tail motion, body prediction, rescue continuation, combat affordability and tests. Verify current protocol 3 inputs, including sonar echoes. Re-run any previous promotion/rejection comparisons under that judge. Existing C++/Python tests can agree with the old model and still be wrong live.

2. **Make the fixed queen the main strategic asset.** Our queen died in 57/70 games, including 48/49 losses; deaths were 25 self, 20 head and 12 other-body collisions. There were 142 queen split actions; 26 left a two-segment queen. In Autarky, the same opening repeatedly drains a length-17 queen through splits before it dies around round 10; Prisoners Dilemma often drains a length-13 queen before death around round 7; Slithery Fight repeatedly loses its queen around round 5. Preserve queen escape capacity and food priority, reduce queen splitting, and make rescue value include survival of the parent queen. Once both queens are dead, switch to longest-snake then total-length objectives. Three round-limit losses were decided by queen length; the other 17 had tied queen lengths and were decided by secondary ranking.

3. **Use the real free movement allowance to escape and collect.** Lengths 5–8 get two free steps, 9–12 get three, and 17–20 get five. Our search caps actions at three and often rejects net-zero sprints for its estimated champion. Use bounded search up to the useful free allowance, evaluate growth after pearls, and consider escape before repeated rescue splits. Keep per-step collision checks and spending limits for paid steps. Free movement is an opportunity, not permission to enter unseen occupancy.

4. **Improve tactical defense and helper attacks using turn order.** 28/29 elimination losses ended in head-to-head death; 18 decisive attacks reached us on enemy step 2 and three on enemy step 3. Across all games, own head deaths numbered 416: enemy-initiated steps 1/2/3/4/5 numbered 139/125/28/1/1; 122 were initiated by our own snake. Many last observations contained enemy heads and empty ordinary alternatives, which does not prove those alternatives would survive. Prefer queen destinations with fewer affordable enemy attack routes, accounting for whether each enemy still acts this round. Give a small non-queen helper a high value for a legal, affordable trade against the enemy queen, even when the enemy queen is short. Protect our own queen and last surviving unit.

5. **Add purposeful portal exploration for helpers and earlier portal escapes.** We made just 16 successful crossings versus opponents’ 6,015. In 1,700 adjacent opportunities, none of the exits were in current vision; 687 pairings were discovered. Of 590 known-pair observations with remembered empty exits, 198 were no more than 16 rounds old. 39 also had at least two remembered empty onward tiles; 35 occurred while a visible ordinary move existed, so the current no-ordinary-move gate blocks proactive use. These are observations, not distinct independent trials, and do not include all existing body/space/enemy checks. In hindsight 37/39 exits were empty and two occupied: relaxing age alone is unsafe. Proposed policy: let small non-queen scouts sample unknown portals, score known exits by resource access, remembered space, enemy reach and exit age, compare them with poor ordinary routes before a trap becomes final, and use fresh ally observations when available. Initially enter one step then reobserve; avoid speculative multi-step blind portal sprints.

   Example: game 849501, Default, snake 3 round 153 length 9: a known north portal had an exit seen empty two rounds earlier, two remembered empty onward tiles, and was actually empty in the replay; the bot chose west. Game 849509, Trophy, snake 3 rounds 85/88 likewise passed known portals for a one-direction ordinary route. These are useful test fixtures, not evidence a portal would have won the game. Game 849566 round 332 is the counterexample: remembered empty exit age 7 with two onward tiles, but the exit was occupied.

   The map named Portals ended 7 wins / 1 loss, with only one successful own crossing across those eight games. Several wins came from keeping our queen alive while opponents lost theirs. Preserve that behavior for a leading queen; spend exploration risk on helpers or when the queen is materially behind. Against Cutlery (849473), our queen remained length 5 and lost to its length-62 queen: passive play cannot win every matchup. [Portal behavior](https://game.battlecode.au/docs/kelp-and-portals).

6. **Turn sonar and population growth into useful coordination.** Sonar currently gets decoded but accepted messages do not update a plan; the bot does not use echoes. Add bounded, authenticated reports for queen identity/status, portal endpoint sightings, resource claims and enemy-queen sightings. Sonar passes through portals, but echoes are delayed aggregate hit counts, not a map or a proof that a particular exit is empty. Use scout reports to reduce uncertainty. Enable small, resource-supported helper expansion after rule fixes; keep queen food priority and avoid congested rescue chains. Opponents collected 55,572 pearls versus our 7,675 and took 701,463 turns versus our 161,139: much of the aggregate difference reflects population, but our food per turn was also lower. [Sonar semantics](https://game.battlecode.au/docs/sonar).

## Validation before any next upload

Use the current judge and held-out seeds on all ten maps in both colors. Promote one isolated feature at a time; record actual queen survival/length, legitimate movement costs, game wins, death types, portal survival and CPU, rather than portal count or total pearls alone. Include direct fixtures for free-sprint tail retention (849542), Autarky/Prisoners Dilemma/Slithery Fight queen openings, and the occupied remembered exit (849566). Replayed observations cannot establish counterfactual wins, and opponent source is not available in these files.

## Every bot bot 3 game

| Game | Map | Opponent | Result | End | Queen ours/enemy | Longest ours/enemy | Own queen death |
|---|---|---|---|---|---|---|---|
| 849470 | Autarky | Cutlery | loss | eliminated | 0/0 | 0/6 | hitSelf r10 length 2 |
| 849471 | Default | Cutlery | loss | eliminated | 0/8 | 0/8 | hitHeadToHead r154 length 4 |
| 849472 | Devil | Cutlery | loss | eliminated | 0/12 | 0/12 | hitHeadToHead r32 length 7 |
| 849473 | Portals | Cutlery | loss | growth deficit | 5/62 | 5/62 | survived |
| 849474 | Prisoners Dilemma | Cutlery | loss | eliminated | 0/0 | 0/4 | hitSelf r7 length 3 |
| 849475 | Queen Of Spades | Cutlery | loss | eliminated | 0/16 | 0/16 | hitHeadToHead r70 length 3 |
| 849476 | Schooltime | Cutlery | loss | eliminated | 0/5 | 0/26 | hitSelf r74 length 2 |
| 849477 | Slithery Fight | Cutlery | loss | eliminated | 0/0 | 0/103 | hitSelf r5 length 3 |
| 849478 | Trauma | Cutlery | loss | eliminated | 0/44 | 0/44 | hitOtherBody r33 length 2 |
| 849479 | Trophy | Cutlery | loss | eliminated | 0/9 | 0/9 | hitHeadToHead r50 length 6 |
| 849490 | Autarky | free trip to sydney pls | loss | eliminated | 0/0 | 0/4 | hitSelf r10 length 2 |
| 849491 | Default | free trip to sydney pls | loss | eliminated | 0/3 | 0/4 | hitHeadToHead r35 length 4 |
| 849492 | Devil | free trip to sydney pls | loss | eliminated | 0/3 | 0/4 | hitHeadToHead r27 length 2 |
| 849493 | Portals | free trip to sydney pls | win | won | 5/0 | 5/32 | survived |
| 849494 | Prisoners Dilemma | free trip to sydney pls | loss | eliminated | 0/0 | 0/6 | hitSelf r7 length 3 |
| 849495 | Queen Of Spades | free trip to sydney pls | loss | eliminated | 0/7 | 0/7 | hitHeadToHead r168 length 6 |
| 849496 | Schooltime | free trip to sydney pls | loss | eliminated | 0/9 | 0/36 | hitHeadToHead r84 length 5 |
| 849497 | Slithery Fight | free trip to sydney pls | loss | growth deficit | 0/0 | 9/62 | hitSelf r5 length 3 |
| 849498 | Trauma | free trip to sydney pls | loss | growth deficit | 0/0 | 34/35 | hitOtherBody r20 length 3 |
| 849499 | Trophy | free trip to sydney pls | loss | eliminated | 0/4 | 0/4 | hitHeadToHead r73 length 10 |
| 849500 | Autarky | Imagine Winning | loss | eliminated | 0/0 | 0/6 | hitSelf r10 length 2 |
| 849501 | Default | Imagine Winning | loss | growth deficit | 0/0 | 7/54 | hitHeadToHead r452 length 3 |
| 849502 | Devil | Imagine Winning | loss | eliminated | 0/0 | 0/8 | hitOtherBody r29 length 2 |
| 849503 | Portals | Imagine Winning | win | won | 5/0 | 5/20 | survived |
| 849504 | Prisoners Dilemma | Imagine Winning | loss | eliminated | 0/0 | 0/7 | hitSelf r7 length 3 |
| 849505 | Queen Of Spades | Imagine Winning | loss | eliminated | 0/0 | 0/5 | hitHeadToHead r159 length 2 |
| 849506 | Schooltime | Imagine Winning | loss | growth deficit | 0/0 | 4/44 | hitHeadToHead r342 length 9 |
| 849507 | Slithery Fight | Imagine Winning | loss | growth deficit | 0/0 | 23/32 | hitSelf r5 length 3 |
| 849508 | Trauma | Imagine Winning | loss | growth deficit | 0/4 | 31/14 | hitOtherBody r199 length 2 |
| 849509 | Trophy | Imagine Winning | loss | eliminated | 0/3 | 0/3 | hitOtherBody r89 length 2 |
| 849530 | Autarky | David Noggins | loss | eliminated | 0/0 | 0/4 | hitSelf r10 length 2 |
| 849531 | Default | David Noggins | loss | eliminated | 0/0 | 0/3 | hitHeadToHead r169 length 7 |
| 849532 | Devil | David Noggins | loss | eliminated | 0/2 | 0/3 | hitOtherBody r40 length 2 |
| 849533 | Portals | David Noggins | win | won | 5/0 | 5/0 | survived |
| 849534 | Prisoners Dilemma | David Noggins | loss | eliminated | 0/0 | 0/4 | hitSelf r7 length 3 |
| 849535 | Queen Of Spades | David Noggins | loss | eliminated | 0/2 | 0/3 | hitHeadToHead r181 length 4 |
| 849536 | Schooltime | David Noggins | loss | growth deficit | 0/0 | 10/19 | hitHeadToHead r277 length 8 |
| 849537 | Slithery Fight | David Noggins | loss | growth deficit | 0/0 | 13/20 | hitSelf r5 length 3 |
| 849538 | Trauma | David Noggins | win | won | 0/0 | 23/18 | hitHeadToHead r270 length 11 |
| 849539 | Trophy | David Noggins | loss | eliminated | 0/0 | 0/4 | hitHeadToHead r86 length 4 |
| 849540 | Autarky | spork | win | won | 0/0 | 7/5 | hitSelf r10 length 2 |
| 849541 | Default | spork | win | won | 10/0 | 10/4 | survived |
| 849542 | Devil | spork | win | won | 0/0 | 5/4 | hitOtherBody r146 length 2 |
| 849543 | Portals | spork | win | won | 5/0 | 5/5 | survived |
| 849544 | Prisoners Dilemma | spork | loss | eliminated | 0/0 | 0/4 | hitSelf r7 length 3 |
| 849545 | Queen Of Spades | spork | win | won | 0/0 | 13/4 | hitSelf r422 length 3 |
| 849546 | Schooltime | spork | win | won | 9/0 | 9/4 | survived |
| 849547 | Slithery Fight | spork | win | won | 0/0 | 7/6 | hitSelf r5 length 3 |
| 849548 | Trauma | spork | win | won | 17/0 | 17/4 | survived |
| 849549 | Trophy | spork | loss | growth deficit | 0/0 | 9/9 | hitOtherBody r128 length 2 |
| 849565 | Autarky | risq-v | loss | growth deficit | 0/0 | 8/17 | hitSelf r10 length 2 |
| 849566 | Default | risq-v | loss | growth deficit | 0/0 | 5/8 | hitHeadToHead r400 length 2 |
| 849567 | Devil | risq-v | loss | eliminated | 0/0 | 0/14 | hitHeadToHead r183 length 9 |
| 849568 | Portals | risq-v | win | won | 3/0 | 6/5 | survived |
| 849569 | Prisoners Dilemma | risq-v | loss | growth deficit | 0/0 | 2/13 | hitSelf r7 length 3 |
| 849570 | Queen Of Spades | risq-v | loss | growth deficit | 0/0 | 2/11 | hitOtherBody r450 length 3 |
| 849571 | Schooltime | risq-v | loss | growth deficit | 0/0 | 11/43 | hitSelf r185 length 2 |
| 849572 | Slithery Fight | risq-v | win | won | 0/0 | 20/12 | hitSelf r5 length 3 |
| 849573 | Trauma | risq-v | win | won | 19/3 | 19/3 | survived |
| 849574 | Trophy | risq-v | loss | growth deficit | 0/5 | 2/10 | hitOtherBody r417 length 2 |
| 849740 | Trauma | code Ex | win | won | 4/0 | 5/0 | survived |
| 849741 | Schooltime | code Ex | win | won | 0/0 | 23/0 | hitSelf r378 length 37 |
| 849742 | Autarky | code Ex | win | won | 0/0 | 16/0 | hitSelf r10 length 2 |
| 849743 | Prisoners Dilemma | code Ex | win | won | 0/0 | 14/3 | hitSelf r7 length 3 |
| 849744 | Portals | code Ex | win | won | 3/0 | 3/0 | survived |
| 849785 | Queen Of Spades | Thermal Throttle | loss | growth deficit | 0/0 | 8/10 | hitOtherBody r152 length 2 |
| 849786 | Devil | Thermal Throttle | loss | growth deficit | 0/0 | 7/27 | hitHeadToHead r59 length 9 |
| 849787 | Portals | Thermal Throttle | win | won | 5/0 | 5/14 | survived |
| 849788 | Prisoners Dilemma | Thermal Throttle | loss | growth deficit | 0/0 | 6/10 | hitSelf r7 length 3 |
| 849789 | Trophy | Thermal Throttle | loss | growth deficit | 0/0 | 5/22 | hitOtherBody r149 length 2 |

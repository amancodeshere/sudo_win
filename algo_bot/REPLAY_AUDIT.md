# Competition replay audit

All 75 downloaded games were inspected event by event. The submitted archive (version 14151) matches the stable source archive byte for byte. The replay body reconstruction agrees with the recorded final count, longest length, and total length for both teams in every game.

Results: **24 wins, 51 losses**. Of the losses, 42 ended by elimination and 9 at the round limit with insufficient length. Our 228 deaths comprise 130 self collisions, 74 head-to-head collisions, and 24 collisions with another body. No invalid-action deaths or timeouts were recorded; maximum measured CPU use was 48,628,124 points.

61 deaths happened in the first ten rounds. Slithery Fight starts one snake with no legal movement; always moving guarantees its loss. Long snakes also become trapped on Autarky and Prisoners Dilemma. Last-turn movement fixes cannot recover these states; rescue splits and earlier trap detection are needed. Opponent populations often grow to dozens through splitting, while our version never splits.

Nine head-to-head pairs were between two of our own snakes, accounting for 18 deaths. A trapped snake's fallback must avoid taking an ally down with it.

The strongest opportunities are legal rescue splits, better growth-versus-space planning, longer enemy threat horizons, and population investment that protects the longest snake. Round-limit scoring uses the longest surviving snake, then total length; reckless population growth is not itself a winning condition.

Rescue splitting passed 29 C++ test cases and 8 Python checks. In 12 judge-sandbox games on Slithery Fight, Autarky, and Prisoners Dilemma (seeds 61/62, both colours), the rescue candidate beat the exact submitted source 9–3. No invalid actions, avoidable visible collisions, or timeouts were detected; maximum CPU consumption across both bots was 47,027,686 points. These are controlled validation games, not new ladder results.

Per-game evidence (original team perspective):

| Game | Map | Opponent | Result | Loss class | Our deaths |
| --- | --- | --- | --- | --- | --- |
| 819740 | Slithery Fight | Uhm? | loss | eliminated | hitSelf: 6, hitOtherBody: 1 |
| 819741 | Devil | Uhm? | loss | eliminated | hitOtherBody: 1, hitHeadToHead: 1, hitSelf: 1 |
| 819742 | Queen Of Spades | Uhm? | win | none | hitOtherBody: 1 |
| 819743 | Default | Uhm? | loss | growth deficit | hitOtherBody: 2, hitHeadToHead: 1 |
| 819744 | Trauma | Uhm? | win | none | none |
| 819770 | Portals | 1 | loss | eliminated | hitSelf: 1, hitHeadToHead: 2 |
| 819771 | Slithery Fight | 1 | loss | eliminated | hitSelf: 5, hitHeadToHead: 2 |
| 819772 | Queen Of Spades | 1 | loss | eliminated | hitHeadToHead: 2 |
| 819773 | Prisoners Dilemma | 1 | loss | eliminated | hitSelf: 1, hitHeadToHead: 2 |
| 819774 | Devil | 1 | loss | eliminated | hitHeadToHead: 3 |
| 819775 | Queen Of Spades | Loremipsum | loss | eliminated | hitSelf: 1, hitHeadToHead: 1 |
| 819776 | Prisoners Dilemma | Loremipsum | loss | eliminated | hitSelf: 2, hitHeadToHead: 1 |
| 819777 | Default | Loremipsum | loss | eliminated | hitSelf: 2, hitHeadToHead: 2 |
| 819778 | Portals | Loremipsum | loss | eliminated | hitSelf: 1, hitHeadToHead: 2 |
| 819779 | Schooltime | Loremipsum | loss | eliminated | hitSelf: 2, hitHeadToHead: 1 |
| 819795 | Devil | Uhm? | loss | eliminated | hitSelf: 1, hitHeadToHead: 2 |
| 819796 | Portals | Uhm? | loss | eliminated | hitSelf: 1, hitHeadToHead: 2 |
| 819797 | Prisoners Dilemma | Uhm? | loss | eliminated | hitSelf: 3 |
| 819798 | Trophy | Uhm? | loss | eliminated | hitSelf: 2 |
| 819799 | Autarky | Uhm? | loss | growth deficit | hitSelf: 2, hitOtherBody: 2 |
| 819820 | Portals | kraken | win | none | none |
| 819821 | Autarky | kraken | win | none | hitSelf: 3 |
| 819822 | Default | kraken | loss | eliminated | hitHeadToHead: 3, hitOtherBody: 1 |
| 819823 | Slithery Fight | kraken | loss | growth deficit | hitSelf: 5, hitHeadToHead: 1 |
| 819824 | Schooltime | kraken | loss | eliminated | hitHeadToHead: 3 |
| 819845 | Trophy | Thermal Throttle | win | none | none |
| 819846 | Queen Of Spades | Thermal Throttle | win | none | none |
| 819847 | Prisoners Dilemma | Thermal Throttle | loss | eliminated | hitSelf: 2, hitOtherBody: 1 |
| 819848 | Autarky | Thermal Throttle | win | none | hitSelf: 2, hitOtherBody: 1, hitHeadToHead: 2 |
| 819849 | Trauma | Thermal Throttle | loss | eliminated | hitHeadToHead: 2 |
| 819897 | Prisoners Dilemma | Uhm? | win | none | hitSelf: 2 |
| 819898 | Slithery Fight | Uhm? | loss | eliminated | hitSelf: 6, hitHeadToHead: 1 |
| 819899 | Trauma | Uhm? | win | none | none |
| 819900 | Autarky | Uhm? | loss | growth deficit | hitSelf: 2, hitHeadToHead: 3 |
| 819901 | Queen Of Spades | Uhm? | loss | eliminated | hitOtherBody: 2 |
| 819913 | Queen Of Spades | Just Reboot Normalize | loss | eliminated | hitOtherBody: 1, hitHeadToHead: 1 |
| 819914 | Default | Just Reboot Normalize | loss | eliminated | hitHeadToHead: 3, hitOtherBody: 1 |
| 819915 | Portals | Just Reboot Normalize | win | none | hitOtherBody: 1 |
| 819916 | Autarky | Just Reboot Normalize | loss | eliminated | hitSelf: 2, hitHeadToHead: 4 |
| 819917 | Schooltime | Just Reboot Normalize | loss | eliminated | hitSelf: 1, hitHeadToHead: 2 |
| 819946 | Devil | wawow830 | loss | eliminated | hitSelf: 2, hitHeadToHead: 1 |
| 819947 | Portals | wawow830 | win | none | hitOtherBody: 1 |
| 819948 | Schooltime | wawow830 | loss | eliminated | hitHeadToHead: 3 |
| 819949 | Slithery Fight | wawow830 | loss | eliminated | hitSelf: 4, hitHeadToHead: 3 |
| 819950 | Queen Of Spades | wawow830 | loss | eliminated | hitSelf: 1, hitHeadToHead: 1 |
| 819984 | Prisoners Dilemma | winner | loss | eliminated | hitSelf: 3, hitOtherBody: 2 |
| 819985 | Default | winner | win | none | hitOtherBody: 1, hitSelf: 1 |
| 819986 | Autarky | winner | win | none | hitSelf: 3 |
| 819987 | Devil | winner | loss | eliminated | hitSelf: 3 |
| 819988 | Trauma | winner | loss | growth deficit | none |
| 819994 | Queen Of Spades | Uhm? | loss | growth deficit | hitSelf: 1 |
| 819995 | Schooltime | Uhm? | loss | eliminated | hitSelf: 2, hitOtherBody: 1 |
| 819996 | Slithery Fight | Uhm? | loss | eliminated | hitSelf: 6, hitOtherBody: 1 |
| 819997 | Devil | Uhm? | loss | eliminated | hitHeadToHead: 1, hitOtherBody: 1, hitSelf: 1 |
| 819998 | Autarky | Uhm? | loss | eliminated | hitSelf: 4, hitHeadToHead: 2 |
| 820024 | Prisoners Dilemma | code Ex | win | none | hitSelf: 1 |
| 820025 | Schooltime | code Ex | win | none | hitSelf: 2 |
| 820026 | Portals | code Ex | loss | eliminated | hitSelf: 1, hitHeadToHead: 2 |
| 820027 | Slithery Fight | code Ex | win | none | hitSelf: 5 |
| 820028 | Autarky | code Ex | win | none | hitSelf: 2 |
| 820039 | Autarky | Loremipsum | loss | growth deficit | hitSelf: 1, hitHeadToHead: 3, hitOtherBody: 1 |
| 820040 | Slithery Fight | Loremipsum | loss | eliminated | hitSelf: 6, hitHeadToHead: 1 |
| 820041 | Trophy | Loremipsum | loss | eliminated | hitSelf: 1, hitHeadToHead: 1 |
| 820042 | Default | Loremipsum | loss | growth deficit | hitHeadToHead: 3 |
| 820043 | Queen Of Spades | Loremipsum | loss | eliminated | hitSelf: 2 |
| 820059 | Slithery Fight | kraken | loss | growth deficit | hitSelf: 5, hitHeadToHead: 1 |
| 820060 | Portals | kraken | win | none | none |
| 820061 | Autarky | kraken | win | none | hitSelf: 2, hitHeadToHead: 1, hitOtherBody: 1 |
| 820062 | Prisoners Dilemma | kraken | win | none | hitSelf: 2 |
| 820063 | Schooltime | kraken | loss | eliminated | hitSelf: 3 |
| 820079 | Portals | slither hither | loss | eliminated | hitSelf: 1, hitHeadToHead: 2 |
| 820080 | Autarky | slither hither | win | none | hitSelf: 1 |
| 820081 | Slithery Fight | slither hither | win | none | hitSelf: 5 |
| 820082 | Prisoners Dilemma | slither hither | win | none | hitSelf: 2 |
| 820083 | Default | slither hither | win | none | hitSelf: 1 |

Detailed JSON evidence, maps, and reconstructed visible turn inputs are generated in `build/replay-audit/`. They are ignored build artifacts. Reconstructed observations are diagnostic fixtures; changed choices alter later observations, so replaying those inputs is not proof of winning the original match. Closed-loop games against source-available opponents are required before promoting changes. Opponent sources are not included in downloaded replays.

## Individual loss explanations

### Game 819740 — Slithery Fight vs Uhm?

Our final snake, ID 6 with 9 segments, died in round 324 from hitOtherBody. Its last observation had 0 known empty adjacent escape directions. 4 of our snakes died in the first ten rounds. 6 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move.

### Game 819741 — Devil vs Uhm?

Our final snake, ID 1 with 4 segments, died in round 132 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819743 — Default vs Uhm?

At round 500, our longest surviving snake was 10 segments versus 15; total length was 10 versus 170. The longer opposing champion decided the loss; extra short snakes alone would not have won it. Growth changes were tested separately and rejected when they lost more controlled games; this matchup still needs stronger farming or champion protection.

### Game 819770 — Portals vs 1

Our final snake, ID 2 with 3 segments, died in round 117 from hitHeadToHead. Snake 0 caused the collision with a recorded 1-step move. Its last observation had 2 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. 1 friendly head-collision pair(s) also lost two allies together; the new fatal fallback protects allied heads. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819771 — Slithery Fight vs 1

Our final snake, ID 7 with 5 segments, died in round 135 from hitHeadToHead. Snake 168 caused the collision with a recorded 2-step move. Its last observation had 2 known empty adjacent escape directions. 4 of our snakes died in the first ten rounds. 5 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819772 — Queen Of Spades vs 1

Our final snake, ID 0 with 2 segments, died in round 133 from hitHeadToHead. Snake 14 caused the collision with a recorded 1-step move. Its last observation had 3 known empty adjacent escape directions. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819773 — Prisoners Dilemma vs 1

Our final snake, ID 5 with 2 segments, died in round 152 from hitHeadToHead. Snake 43 caused the collision with a recorded 1-step move. Its last observation had 2 known empty adjacent escape directions. 1 of our snakes died in the first ten rounds. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819774 — Devil vs 1

Our final snake, ID 0 with 10 segments, died in round 80 from hitHeadToHead. Its last observation had 0 known empty adjacent escape directions. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819775 — Queen Of Spades vs Loremipsum

Our final snake, ID 1 with 15 segments, died in round 381 from hitHeadToHead. Snake 8 caused the collision with a recorded 1-step move. Its last observation had 3 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819776 — Prisoners Dilemma vs Loremipsum

Our final snake, ID 4 with 11 segments, died in round 176 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 1 of our snakes died in the first ten rounds. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819777 — Default vs Loremipsum

Our final snake, ID 3 with 16 segments, died in round 349 from hitHeadToHead. Snake 29 caused the collision with a recorded 2-step move. Its last observation had 2 known empty adjacent escape directions. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819778 — Portals vs Loremipsum

Our final snake, ID 2 with 3 segments, died in round 117 from hitHeadToHead. Snake 0 caused the collision with a recorded 1-step move. Its last observation had 2 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. 1 friendly head-collision pair(s) also lost two allies together; the new fatal fallback protects allied heads. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819779 — Schooltime vs Loremipsum

Our final snake, ID 5 with 41 segments, died in round 277 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819795 — Devil vs Uhm?

Our final snake, ID 3 with 16 segments, died in round 274 from hitHeadToHead. Its last observation had 0 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819796 — Portals vs Uhm?

Our final snake, ID 2 with 3 segments, died in round 117 from hitHeadToHead. Snake 0 caused the collision with a recorded 1-step move. Its last observation had 2 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. 1 friendly head-collision pair(s) also lost two allies together; the new fatal fallback protects allied heads. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819797 — Prisoners Dilemma vs Uhm?

Our final snake, ID 5 with 20 segments, died in round 349 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 1 of our snakes died in the first ten rounds. 3 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move.

### Game 819798 — Trophy vs Uhm?

Our final snake, ID 2 with 38 segments, died in round 199 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move.

### Game 819799 — Autarky vs Uhm?

At round 500, our longest surviving snake was 12 segments versus 18; total length was 20 versus 199. The longer opposing champion decided the loss; extra short snakes alone would not have won it. 1 of our snakes died in the first ten rounds. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Growth changes were tested separately and rejected when they lost more controlled games; this matchup still needs stronger farming or champion protection.

### Game 819822 — Default vs kraken

Our final snake, ID 2 with 24 segments, died in round 389 from hitOtherBody. Its last observation had 0 known empty adjacent escape directions. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819823 — Slithery Fight vs kraken

At round 500, our longest surviving snake was 4 segments versus 20; total length was 4 versus 184. The longer opposing champion decided the loss; extra short snakes alone would not have won it. 4 of our snakes died in the first ten rounds. 5 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Growth changes were tested separately and rejected when they lost more controlled games; this matchup still needs stronger farming or champion protection.

### Game 819824 — Schooltime vs kraken

Our final snake, ID 4 with 11 segments, died in round 160 from hitHeadToHead. Snake 38 caused the collision with a recorded 2-step move. Its last observation had 2 known empty adjacent escape directions. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819847 — Prisoners Dilemma vs Thermal Throttle

Our final snake, ID 3 with 7 segments, died in round 379 from hitOtherBody. Its last observation had 0 known empty adjacent escape directions. 1 of our snakes died in the first ten rounds. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move.

### Game 819849 — Trauma vs Thermal Throttle

Our final snake, ID 1 with 17 segments, died in round 300 from hitHeadToHead. Snake 3 caused the collision with a recorded 1-step move. Its last observation had 2 known empty adjacent escape directions. 1 friendly head-collision pair(s) also lost two allies together; the new fatal fallback protects allied heads. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819898 — Slithery Fight vs Uhm?

Our final snake, ID 10 with 34 segments, died in round 439 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 4 of our snakes died in the first ten rounds. 6 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819900 — Autarky vs Uhm?

At round 500, our longest surviving snake was 7 segments versus 21; total length was 7 versus 234. The longer opposing champion decided the loss; extra short snakes alone would not have won it. 1 of our snakes died in the first ten rounds. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. 1 friendly head-collision pair(s) also lost two allies together; the new fatal fallback protects allied heads. Growth changes were tested separately and rejected when they lost more controlled games; this matchup still needs stronger farming or champion protection.

### Game 819901 — Queen Of Spades vs Uhm?

Our final snake, ID 3 with 12 segments, died in round 381 from hitOtherBody. Its last observation had 0 known empty adjacent escape directions.

### Game 819913 — Queen Of Spades vs Just Reboot Normalize

Our final snake, ID 1 with 12 segments, died in round 375 from hitHeadToHead. Snake 35 caused the collision with a recorded 3-step move. Its last observation had 2 known empty adjacent escape directions. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819914 — Default vs Just Reboot Normalize

Our final snake, ID 6 with 17 segments, died in round 265 from hitOtherBody. Its last observation had 0 known empty adjacent escape directions. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819916 — Autarky vs Just Reboot Normalize

Our final snake, ID 6 with 11 segments, died in round 441 from hitHeadToHead. Snake 72 caused the collision with a recorded 4-step move. Its last observation had 2 known empty adjacent escape directions. 1 of our snakes died in the first ten rounds. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. 1 friendly head-collision pair(s) also lost two allies together; the new fatal fallback protects allied heads. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819917 — Schooltime vs Just Reboot Normalize

Our final snake, ID 1 with 14 segments, died in round 153 from hitHeadToHead. Snake 59 caused the collision with a recorded 4-step move. Its last observation had 2 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819946 — Devil vs wawow830

Our final snake, ID 4 with 17 segments, died in round 98 from hitHeadToHead. Snake 65 caused the collision with a recorded 2-step move. Its last observation had 2 known empty adjacent escape directions. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819948 — Schooltime vs wawow830

Our final snake, ID 2 with 24 segments, died in round 194 from hitHeadToHead. Snake 3 caused the collision with a recorded 2-step move. Its last observation had 1 known empty adjacent escape directions. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819949 — Slithery Fight vs wawow830

Our final snake, ID 7 with 15 segments, died in round 198 from hitHeadToHead. Snake 228 caused the collision with a recorded 2-step move. Its last observation had 2 known empty adjacent escape directions. 4 of our snakes died in the first ten rounds. 4 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819950 — Queen Of Spades vs wawow830

Our final snake, ID 0 with 27 segments, died in round 361 from hitHeadToHead. Snake 141 caused the collision with a recorded 2-step move. Its last observation had 2 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819984 — Prisoners Dilemma vs winner

Our final snake, ID 2 with 13 segments, died in round 173 from hitOtherBody. Its last observation had 0 known empty adjacent escape directions. 1 of our snakes died in the first ten rounds. 3 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move.

### Game 819987 — Devil vs winner

Our final snake, ID 1 with 31 segments, died in round 129 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 3 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move.

### Game 819988 — Trauma vs winner

At round 500, our longest surviving snake was 24 segments versus 38; total length was 36 versus 38. The longer opposing champion decided the loss; extra short snakes alone would not have won it. Growth changes were tested separately and rejected when they lost more controlled games; this matchup still needs stronger farming or champion protection.

### Game 819994 — Queen Of Spades vs Uhm?

At round 500, our longest surviving snake was 11 segments versus 11; total length was 11 versus 92. The longest-snake scores tied, so total length decided the loss. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Growth changes were tested separately and rejected when they lost more controlled games; this matchup still needs stronger farming or champion protection.

### Game 819995 — Schooltime vs Uhm?

Our final snake, ID 1 with 16 segments, died in round 496 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move.

### Game 819996 — Slithery Fight vs Uhm?

Our final snake, ID 6 with 7 segments, died in round 398 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 4 of our snakes died in the first ten rounds. 6 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move.

### Game 819997 — Devil vs Uhm?

Our final snake, ID 1 with 12 segments, died in round 180 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 819998 — Autarky vs Uhm?

Our final snake, ID 2 with 6 segments, died in round 343 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 1 of our snakes died in the first ten rounds. 4 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. 1 friendly head-collision pair(s) also lost two allies together; the new fatal fallback protects allied heads. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 820026 — Portals vs code Ex

Our final snake, ID 2 with 3 segments, died in round 117 from hitHeadToHead. Snake 0 caused the collision with a recorded 1-step move. Its last observation had 2 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. 1 friendly head-collision pair(s) also lost two allies together; the new fatal fallback protects allied heads. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 820039 — Autarky vs Loremipsum

At round 500, our longest surviving snake was 10 segments versus 16; total length was 10 versus 129. The longer opposing champion decided the loss; extra short snakes alone would not have won it. 1 of our snakes died in the first ten rounds. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Growth changes were tested separately and rejected when they lost more controlled games; this matchup still needs stronger farming or champion protection.

### Game 820040 — Slithery Fight vs Loremipsum

Our final snake, ID 10 with 9 segments, died in round 262 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 4 of our snakes died in the first ten rounds. 6 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 820041 — Trophy vs Loremipsum

Our final snake, ID 3 with 19 segments, died in round 182 from hitHeadToHead. Snake 10 caused the collision with a recorded 2-step move. Its last observation had 1 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

### Game 820042 — Default vs Loremipsum

At round 500, our longest surviving snake was 9 segments versus 17; total length was 9 versus 148. The longer opposing champion decided the loss; extra short snakes alone would not have won it. 1 of our snakes died in the first ten rounds. Growth changes were tested separately and rejected when they lost more controlled games; this matchup still needs stronger farming or champion protection.

### Game 820043 — Queen Of Spades vs Loremipsum

Our final snake, ID 1 with 19 segments, died in round 166 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 2 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move.

### Game 820059 — Slithery Fight vs kraken

At round 500, our longest surviving snake was 4 segments versus 13; total length was 4 versus 115. The longer opposing champion decided the loss; extra short snakes alone would not have won it. 4 of our snakes died in the first ten rounds. 5 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. Growth changes were tested separately and rejected when they lost more controlled games; this matchup still needs stronger farming or champion protection.

### Game 820063 — Schooltime vs kraken

Our final snake, ID 3 with 24 segments, died in round 246 from hitSelf. Its last observation had 0 known empty adjacent escape directions. 3 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move.

### Game 820079 — Portals vs slither hither

Our final snake, ID 2 with 3 segments, died in round 117 from hitHeadToHead. Snake 0 caused the collision with a recorded 1-step move. Its last observation had 2 known empty adjacent escape directions. 1 self-collision deaths show the importance of reversing trapped tails through legal rescue splits instead of forcing a move. 1 friendly head-collision pair(s) also lost two allies together; the new fatal fallback protects allied heads. Broader sprint-threat policies were trialled but reduced overall wins, so they were not promoted; this remains a tactical weakness.

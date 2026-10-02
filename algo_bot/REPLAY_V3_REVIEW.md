# Uploaded version 3 replay review

Submission **14465**, named `bot bot v2`, was uploaded and compiled successfully on 2 October 2026; the server automatically made it active. Its source ZIP is `build/submission-competition-v3.zip` (SHA256 `77294fe08562e1c62fa54c6010b8f1104dbf92a5c6d871f9e62d569f85125651`). This supersedes the earlier upload status in `STRATEGY_UPDATES.md`.

The download snapshot contains 318 game files: 90 newly downloaded, 228 already present, zero unavailable or failed. Replay bot IDs identify **70 games belonging to 14465**, ending **16 wins / 54 losses**. The remaining 20 new downloads belong to older submissions and are excluded. Every selected replay was processed event by event, and final count, longest length and total length agree with the judge for both teams.

**34 losses were eliminations; 20 were growth deficits.** Our 2,022 deaths include 878 self-collisions, 803 other-body collisions and 341 head collisions. All self/other-body deaths had no empty, currently visible first-step alternative in their final observation; avoiding the earlier trap matters more than changing the final fallback. No timeout or invalid-action deaths occurred; peak CPU use was 36,289,002 / 100,000,000 points.

Enemy-initiated head kills occurred at executed steps 1–6: **81, 141, 58, 15, 4, 1**, respectively. These count successful movement updates before the fatal step, rather than the declared action length. The other 41 head deaths were initiated by one of our own snakes. An attack being possible is not evidence that every alternative was safe.

Portal observations expose the current limitation: 1,578 adjacent portal opportunities, all with exits outside current vision; 597 had discovered partner edges. Of those 597, 453 had previously seen empty exits, but only 15 observations were at most two rounds old and 192 were at most sixteen rounds old. The replay contains actual hidden occupancy for diagnosis; decisions must use remembered observations only. Portals won all eight games on the map named Portals, so unrestricted teleporting is not justified merely to increase crossings.

## Priorities

1. Avoid enemy sprint routes that visible length and pearl income can fund, before trading safety for an attractive pearl. Retain uncertainty for partially observed enemies.
2. Prevent growth into spaces that become unsafe after collecting a pearl; distinguish forced rescue loops from productive growth.
3. Add a separate portal escape policy for known pairs with remembered empty exits and plausible onward routes. Keep stale occupancy uncertain and compare risk with available ordinary escapes.

## Every game

| Game | Map | Opponent | Result | Diagnosis | Final longest ours / opponent | Our / opponent splits |
| --- | --- | --- | --- | --- | --- | --- |
| 843208 | Autarky | Sponge(Albert and Bob) | loss | eliminated round 179: hitHeadToHead at enemy step 2 | 0 / 4 | 11 / 143 |
| 843209 | Default | Sponge(Albert and Bob) | loss | eliminated round 141: hitHeadToHead at enemy step 2 | 0 / 5 | 0 / 73 |
| 843210 | Devil | Sponge(Albert and Bob) | loss | eliminated round 94: hitHeadToHead at enemy step 2 | 0 / 4 | 1 / 45 |
| 843211 | Portals | Sponge(Albert and Bob) | win | survived / won | 5 / 38 | 0 / 520 |
| 843212 | Prisoners Dilemma | Sponge(Albert and Bob) | loss | eliminated round 153: hitHeadToHead at enemy step 1 | 0 / 3 | 9 / 49 |
| 843213 | Queen Of Spades | Sponge(Albert and Bob) | loss | eliminated round 122: hitHeadToHead at enemy step 2 | 0 / 5 | 0 / 32 |
| 843214 | Schooltime | Sponge(Albert and Bob) | loss | eliminated round 77: hitHeadToHead at enemy step 3 | 0 / 4 | 0 / 48 |
| 843215 | Slithery Fight | Sponge(Albert and Bob) | loss | round limit: longest 3 vs 85 | 3 / 85 | 60 / 829 |
| 843216 | Trauma | Sponge(Albert and Bob) | loss | eliminated round 328: hitHeadToHead at enemy step 2 | 0 / 25 | 1 / 366 |
| 843217 | Trophy | Sponge(Albert and Bob) | loss | eliminated round 71: hitHeadToHead at enemy step 3 | 0 / 4 | 0 / 27 |
| 843248 | Schooltime | Uhm? | win | survived / won | 12 / 26 | 8 / 83 |
| 843249 | Trauma | Uhm? | win | survived / won | 15 / 12 | 7 / 6 |
| 843250 | Slithery Fight | Uhm? | loss | round limit: longest 14 vs 25 | 14 / 25 | 355 / 572 |
| 843251 | Portals | Uhm? | win | survived / won | 5 / 9 | 0 / 97 |
| 843252 | Autarky | Uhm? | loss | round limit: longest 12 vs 53 | 12 / 53 | 23 / 63 |
| 843318 | Portals | SuitedConnectors | win | survived / won | 5 / 17 | 0 / 271 |
| 843319 | Queen Of Spades | SuitedConnectors | loss | round limit: longest 29 vs 30 | 29 / 30 | 0 / 61 |
| 843320 | Trophy | SuitedConnectors | win | survived / won | 23 / 25 | 42 / 316 |
| 843321 | Prisoners Dilemma | SuitedConnectors | win | survived / won | 11 / 0 | 12 / 117 |
| 843322 | Slithery Fight | SuitedConnectors | win | survived / won | 33 / 17 | 325 / 1310 |
| 844028 | Autarky | larpmaxxing | loss | eliminated round 342: hitHeadToHead at enemy step 1 | 0 / 17 | 11 / 321 |
| 844029 | Default | larpmaxxing | loss | eliminated round 190: hitHeadToHead at enemy step 3 | 0 / 10 | 0 / 111 |
| 844030 | Devil | larpmaxxing | loss | round limit: longest 2 vs 54 | 2 / 54 | 7 / 734 |
| 844031 | Portals | larpmaxxing | win | survived / won | 5 / 46 | 0 / 349 |
| 844032 | Prisoners Dilemma | larpmaxxing | loss | eliminated round 238: hitHeadToHead at enemy step 1 | 0 / 5 | 7 / 187 |
| 844033 | Queen Of Spades | larpmaxxing | loss | eliminated round 304: hitHeadToHead at enemy step 2 | 0 / 9 | 0 / 121 |
| 844034 | Schooltime | larpmaxxing | loss | eliminated round 131: hitHeadToHead at enemy step 3 | 0 / 22 | 1 / 115 |
| 844035 | Slithery Fight | larpmaxxing | loss | round limit: longest 3 vs 80 | 3 / 80 | 61 / 1334 |
| 844036 | Trauma | larpmaxxing | loss | round limit: longest 19 vs 55 | 19 / 55 | 1 / 67 |
| 844037 | Trophy | larpmaxxing | loss | eliminated round 188: hitHeadToHead at enemy step 3 | 0 / 9 | 0 / 113 |
| 844058 | Autarky | nsw seng | loss | round limit: longest 3 vs 41 | 3 / 41 | 11 / 165 |
| 844059 | Default | nsw seng | loss | eliminated round 274: hitHeadToHead at enemy step 3 | 0 / 7 | 1 / 104 |
| 844060 | Devil | nsw seng | loss | eliminated round 103: hitHeadToHead at enemy step 2 | 0 / 7 | 0 / 66 |
| 844061 | Portals | nsw seng | win | survived / won | 5 / 29 | 0 / 306 |
| 844062 | Prisoners Dilemma | nsw seng | loss | round limit: longest 4 vs 28 | 4 / 28 | 14 / 208 |
| 844063 | Queen Of Spades | nsw seng | loss | eliminated round 207: hitHeadToHead at enemy step 2 | 0 / 9 | 0 / 75 |
| 844064 | Schooltime | nsw seng | loss | round limit: longest 5 vs 36 | 5 / 36 | 1 / 175 |
| 844065 | Slithery Fight | nsw seng | loss | round limit: longest 7 vs 37 | 7 / 37 | 209 / 677 |
| 844066 | Trauma | nsw seng | win | survived / won | 17 / 11 | 0 / 18 |
| 844067 | Trophy | nsw seng | loss | eliminated round 89: hitHeadToHead at enemy step 2 | 0 / 45 | 0 / 26 |
| 844088 | Autarky | Cache me outside | loss | eliminated round 192: hitHeadToHead at enemy step 2 | 0 / 4 | 7 / 122 |
| 844089 | Default | Cache me outside | loss | eliminated round 173: hitHeadToHead at enemy step 2 | 0 / 3 | 1 / 76 |
| 844090 | Devil | Cache me outside | loss | eliminated round 352: hitHeadToHead at enemy step 2 | 0 / 4 | 2 / 525 |
| 844091 | Portals | Cache me outside | win | survived / won | 5 / 47 | 0 / 581 |
| 844092 | Prisoners Dilemma | Cache me outside | loss | eliminated round 151: hitHeadToHead at enemy step 1 | 0 / 4 | 6 / 105 |
| 844093 | Queen Of Spades | Cache me outside | loss | eliminated round 227: hitHeadToHead at enemy step 2 | 0 / 4 | 0 / 90 |
| 844094 | Schooltime | Cache me outside | loss | eliminated round 329: hitHeadToHead at enemy step 2 | 0 / 24 | 0 / 178 |
| 844095 | Slithery Fight | Cache me outside | loss | round limit: longest 29 vs 75 | 29 / 75 | 21 / 1218 |
| 844096 | Trauma | Cache me outside | loss | round limit: longest 17 vs 40 | 17 / 40 | 0 / 556 |
| 844097 | Trophy | Cache me outside | loss | eliminated round 97: hitHeadToHead at enemy step 1 | 0 / 4 | 0 / 47 |
| 844098 | Autarky | NeungzAI | loss | eliminated round 191: hitSelf | 0 / 18 | 11 / 247 |
| 844099 | Default | NeungzAI | loss | eliminated round 233: hitHeadToHead at enemy step 1 | 0 / 25 | 1 / 79 |
| 844100 | Devil | NeungzAI | loss | eliminated round 112: hitHeadToHead at enemy step 2 | 0 / 11 | 5 / 67 |
| 844101 | Portals | NeungzAI | win | survived / won | 5 / 8 | 0 / 368 |
| 844102 | Prisoners Dilemma | NeungzAI | loss | eliminated round 153: hitHeadToHead at enemy step 2 | 0 / 12 | 8 / 118 |
| 844103 | Queen Of Spades | NeungzAI | loss | eliminated round 146: hitHeadToHead at enemy step 2 | 0 / 5 | 0 / 52 |
| 844104 | Schooltime | NeungzAI | loss | eliminated round 247: hitHeadToHead at enemy step 2 | 0 / 7 | 0 / 111 |
| 844105 | Slithery Fight | NeungzAI | loss | round limit: longest 11 vs 26 | 11 / 26 | 290 / 1291 |
| 844106 | Trauma | NeungzAI | loss | round limit: longest 5 vs 16 | 5 / 16 | 12 / 21 |
| 844107 | Trophy | NeungzAI | loss | eliminated round 206: hitHeadToHead at enemy step 2 | 0 / 39 | 10 / 89 |
| 844143 | Autarky | Submarine.exe | loss | round limit: longest 16 vs 27 | 16 / 27 | 35 / 291 |
| 844144 | Default | Submarine.exe | loss | round limit: longest 11 vs 34 | 11 / 34 | 31 / 9 |
| 844145 | Devil | Submarine.exe | loss | eliminated round 414: hitHeadToHead at enemy step 1 | 0 / 7 | 7 / 706 |
| 844146 | Portals | Submarine.exe | win | survived / won | 6 / 11 | 9 / 637 |
| 844147 | Prisoners Dilemma | Submarine.exe | loss | round limit: longest 9 vs 13 | 9 / 13 | 20 / 271 |
| 844148 | Queen Of Spades | Submarine.exe | loss | round limit: longest 7 vs 20 | 7 / 20 | 1 / 71 |
| 844149 | Schooltime | Submarine.exe | win | survived / won | 16 / 15 | 171 / 158 |
| 844150 | Slithery Fight | Submarine.exe | loss | round limit: longest 9 vs 22 | 9 / 22 | 400 / 780 |
| 844151 | Trauma | Submarine.exe | win | survived / won | 20 / 13 | 0 / 112 |
| 844152 | Trophy | Submarine.exe | loss | eliminated round 462: hitHeadToHead at enemy step 4 | 0 / 12 | 0 / 326 |

Detailed per-turn inputs, portal knowledge and executed collision steps are in `build/live-v3-audit/`. Downloaded opponents do not include their source code; replay inputs are diagnostic observations, not counterfactual wins. Local closed-loop comparisons and sandbox CPU checks are required before promoting changes.

Reproduce:

```sh
set -a
source .env
set +a
python3 algo_bot/util/download_replays.py
python3 algo_bot/util/analyze_replays.py --submission 14465 --output build/live-v3-audit
```

## First controlled trial

Funded two-step attacks now carry a separate capability estimate. Visible
segments and intermediate pearl income fund the path; missing body parts do
not prove an extra sprint can be paid. Turning that estimate into hard movement
priority lost **7–13** against the uploaded source on all ten competition maps,
seed 201, both colours (`build/validation/funded-evasion-native20`). This was
unmetered diagnostic play with no engine/protocol failures. Broad priority is
therefore disabled by default (`enable_funded_sprint_priority = false`) and
available only as an experimental policy. It is not a promoted improvement.

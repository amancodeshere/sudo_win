# Strategy updates — 2 October 2026

Historical review before upload 14465. The candidate described here was later
uploaded as `bot bot v2` and automatically activated. See
[the latest 85-game review](REPLAY_V3_REVIEW.md) and
[validation](VALIDATION.md) for the new conservative package and current status.

## Competition evidence

The user activated submission 14399 (`bot bot`, version 2) after the previous
local work. Its first 20 games against the leaderboard's top two teams ended
2–18: one Portals win against each. Both series used all ten current competition
maps. This session downloaded the remaining 153 available replays; there are
now 228 local game files. The new version's 20 games were audited separately in
`build/live-v2-audit/`, including final standings checks for both teams.

Those games contain 15 elimination losses and three growth losses, with 118
head-to-head deaths, 69 self-collisions and 30 other-body collisions. Leaders
frequently split into small collectors and attackers. This is a larger tactical
problem than increasing our champion's length alone. Diagnosed enemy attacks
include 60 two-step, 29 one-step, 21 three-step and three four-step head kills.
Counts use the actual acting dragon and its recorded action, not proximity.
The other five head deaths were one-step trades initiated by our bot; none of
these 20 games contained friendly head-to-head collisions.

| Game | Map | Opponent | Outcome | Our / enemy splits | Final longest: ours / enemy |
| --- | --- | --- | --- | --- | --- |
| 839290 | Autarky | 中国必须人能飞😡 | loss | 9 / 121 | 0 / 4 |
| 839291 | Default | 中国必须人能飞😡 | loss | 1 / 206 | 0 / 4 |
| 839292 | Devil | 中国必须人能飞😡 | loss | 1 / 44 | 0 / 4 |
| 839293 | Portals | 中国必须人能飞😡 | win | 0 / 479 | 5 / 42 |
| 839294 | Prisoners Dilemma | 中国必须人能飞😡 | loss | 6 / 229 | 0 / 5 |
| 839295 | Queen Of Spades | 中国必须人能飞😡 | loss | 0 / 76 | 0 / 3 |
| 839296 | Schooltime | 中国必须人能飞😡 | loss | 0 / 43 | 0 / 3 |
| 839297 | Slithery Fight | 中国必须人能飞😡 | loss | 73 / 339 | 16 / 67 |
| 839298 | Trauma | 中国必须人能飞😡 | loss | 4 / 326 | 25 / 52 |
| 839299 | Trophy | 中国必须人能飞😡 | loss | 0 / 79 | 0 / 4 |
| 839485 | Autarky | Sponge(Albert and Bob) | loss | 11 / 104 | 0 / 6 |
| 839486 | Default | Sponge(Albert and Bob) | loss | 1 / 76 | 0 / 3 |
| 839487 | Devil | Sponge(Albert and Bob) | loss | 7 / 170 | 0 / 6 |
| 839488 | Portals | Sponge(Albert and Bob) | win | 6 / 518 | 6 / 43 |
| 839489 | Prisoners Dilemma | Sponge(Albert and Bob) | loss | 6 / 144 | 0 / 4 |
| 839490 | Queen Of Spades | Sponge(Albert and Bob) | loss | 0 / 20 | 0 / 4 |
| 839491 | Schooltime | Sponge(Albert and Bob) | loss | 0 / 37 | 0 / 4 |
| 839492 | Slithery Fight | Sponge(Albert and Bob) | loss | 65 / 757 | 13 / 73 |
| 839493 | Trauma | Sponge(Albert and Bob) | loss | 0 / 246 | 0 / 10 |
| 839494 | Trophy | Sponge(Albert and Bob) | loss | 0 / 50 | 0 / 4 |

## Implemented policies and promotion decisions

- Rescue splits search larger reversed-tail children first, keeping more of the
  endangered champion's length when an escaping child can survive the bounded
  search. The parent may still die. Unknown tails retain the legal last-resort
  two-segment attempt only when no legal visible movement exists.
- Small collectors discount resource routes claimed by a closer visible ally.
  Claims follow known legal edges, reject current occupants, and ignore stale
  allies. Growing snakes keep priority unless an ally is visibly longer.
- A chamber assessment detects a known entrance sealed by the neck, with fewer
  cells than the snake's length. It refuses to certify unknown exits, missing
  portal partners, incomplete bodies or chambers containing tail segments that
  can vacate. Giving this estimate hard priority regressed Slithery Fight;
  `enable_pocket_priority` is false in the upload profile.
- Optional sprint forecasts extend legal simulation to five steps under a
  192-node budget. Visible body segments and collected pearls pay the costs;
  partial heads are not assigned a large imaginary sprint budget. Longer
  routes get a small soft score instead of direct-attack priority. The combined
  strategy regressed; `enable_long_sprint_threats` is false in the upload profile.
- Optional expansion can create a two-segment collector from a complete body
  of 4–12 segments before round 280, below a 24-unit soft cap. Both new snakes
  need two six-step escape routes, adequate visible space, separate resource
  opportunities and unthreatened stationary heads. An existing local champion
  does not invest, and a solitary champion only does so before round 80.
  Expansion tied its comparison and remains disabled in the upload profile.

`stable` and `no-sprint` disable all three new experimental flags. `growth`
enables only selective expansion; `experimental` enables those flags plus the
older investment policy. Rescue splitting remains available in every profile.

## Validation

All comparisons below use immutable selected-source snapshots and both colours.
Native runs are diagnostic and do not verify judge CPU limits. Single held-out
seeds and small samples cannot establish superiority over live competitors.

| Comparison | Evidence | Decision |
| --- | --- | --- |
| Larger rescue child vs active version, four maps, seeds 91–93 | 13–11, native | Retain |
| Larger rescue child vs active version, Trauma/Autarky, seed 98 | 4–0, judge sandbox; 36,398,607 max points | Retain |
| Yield resources to every closer ally, all 15 toolkit maps, seed 94 | 11–19, native | Reject |
| Preserve long-snake priority, vs rescue candidate, all 15 maps, seed 97 | 19–11, native | Retain revised coordination |
| Long sprint forecast vs unrestricted coordination, four maps, seeds 95/96 | 9–7, native | Insufficient for promotion |
| Early conservative expansion, four maps, seeds 99/100 | 8–8, native | Keep experimental |
| Stronger isolated expansion, ten competition maps, seed 105 | 10–10, native | Keep experimental |
| Combined priorities and forecasts, all 15 toolkit maps, seed 102 | 15–15, native | No demonstrated benefit |
| Combined priorities and forecasts, ten competition maps, seed 103 | 8–12, judge sandbox; 36,683,067 max points | Reject upload configuration |
| Pocket priority without extended forecasts, ten competition maps, seed 103 | 8–12, judge sandbox; 36,343,846 max points | Reject upload configuration |
| Rescue + revised coordination without experimental policies, seed 103 | 10–10 native and exact final-archive judge run on ten competition maps; candidate peak 36,187,732 | Final configuration |
| Expansion fixture, both colours, each repeated | Four judge runs; identical repeat hashes; legal split and same-round child action | Mechanical validation only |

The exact final archive's judge results and checksums are recorded in
`VALIDATION.md`. This candidate has not been uploaded or activated by Codex.
The live 2–18 record belongs to version 2, not the new archive. No claim of
beating the top bots is justified until the new candidate plays them.

## Upload and evaluate the exact candidate

Run from the repository root:

```bash
set -a
source .env
set +a
curl --fail-with-body "$UNSWBC_SERVER/api/v1/submissions" \
  -H "Authorization: Bearer $UNSWBC_KEY" -H "Origin: $UNSWBC_SERVER" \
  -F 'name=bot bot' \
  -F 'description=i will sudo win.. sudo win.. sundo win...' \
  -F 'language=cpp' \
  -F 'zip=@build/submission-competition-v3.zip;type=application/zip'
```

Use the returned ID to inspect the build status/log through
`GET /api/v1/submissions/ID`. Once compilation succeeds, activate that ID:

```bash
COMPETITION_SUBMISSION_ID=12345 # Replace with the returned ID.
curl --fail-with-body -X POST \
  "$UNSWBC_SERVER/api/v1/submissions/$COMPETITION_SUBMISSION_ID/activate" \
  -H "Authorization: Bearer $UNSWBC_KEY"
```

The live benchmark helper reads current leaders/maps, verifies this submission
is active and writes the intended unranked requests. Without `--queue` it only
plans; it never uploads or activates a bot.

```bash
python3 algo_bot/util/live_benchmark.py --submission "$COMPETITION_SUBMISSION_ID" \
  --output "build/live-plan-$COMPETITION_SUBMISSION_ID.json"
# Explicitly queue 20 unranked games against the top two eligible leaders:
python3 algo_bot/util/live_benchmark.py --submission "$COMPETITION_SUBMISSION_ID" \
  --output "build/live-queued-$COMPETITION_SUBMISSION_ID.json" --queue
```

The server enforces the hourly limit, including earlier requests. Ambiguous POST
failures are not retried, to avoid duplicate challenges. Inspect saved responses
and `/api/v1/battles/ID` for completion, then rerun `download_replays.py` and audit
those games. The API controls colours; the planner does not assume that one
series covers both. Queue further series for the missing colours if necessary.

Local `benchmark.py --both-colours` tests both positions explicitly.

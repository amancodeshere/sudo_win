# Competition v5 validation — 2 October 2026

The requested replay improvements are implemented and committed independently.
The upload-ready package is `build/submission-competition-v5.zip`; no submission
or live challenge was made during this task. Bot bot 3 (submission 14545, version
4) remains the historical comparison baseline. No live activation was changed.

## Implemented behavior

- Fixed queen identity, food priority, parent-preserving rescues and deliberate
  preservation of a secondary contender when a queen escape cannot be certified.
- Live `ceil(action-start length / 4)` free allowance in movement simulation,
  combat funding, remembered body predictions and independent collision checks.
- Bounded free sprint beam search; funded enemy attacks are separated by turn
  order, with stronger protection for queens and last survivors.
- Earlier remembered portal escapes for helpers; small helpers can sample unknown
  portals after local resource exhaustion, with a cooldown and complete body
  requirement. Queens keep cautious escape/rescue precedence. Crossings are one
  step before observing the exit again.
- Bounded team sonar reports for queen status, food claims, portal topology and
  enemy-queen sightings; thirteen-bit identities, checked fields and expiry.
  Explicit team separation handles mirrored opponents. Reports stay advisory;
  neither tagged messages nor aggregate echoes certify empty remote exits.
- Funded, visible enemy-queen removal by a length-two/three helper with a teammate.
  Our queen, last survivor and incomplete bodies cannot be sacrificed. Broad
  nonqueen trades and other experimental priorities remain disabled.
- Resource-supported expansion stops at eight units and round 280. Independent
  legal body routes, escape space and queen claims determine whether investment
  is permitted. Stale ally estimates expire; late protected snakes retain their
  restriction on paid zero-growth sprints.

## Correctness and runtime checks

55 C++ cases / 362 assertions passed with Apple Clang Release, LLVM Clang Release,
GCC 15 Release and Clang Debug with AddressSanitizer / UndefinedBehaviorSanitizer.
Strict warnings are errors. All 16 Python utility tests passed, including exact
one-feature ablations, deterministic packaging and independent head-trade checks.

Official toolkit: **unswbc 1.2.5**, Wasmtime 49.0.0. Engine SHA256:
`26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`.
`util/verify_rules.py` independently verifies both-color queen scoring, a dead
queen remaining zero, free steps and frozen action-start allowance. Every
benchmark runs these probes before matches. Old 1.2.2 results are historical.

| Comparison | Games/runs | Result | Candidate errors | Peak CPU points |
| --- | ---: | --- | ---: | ---: |
| Combined strategy vs bot bot 3, seed 401, all ten maps / both colors | 20 | 12–8 | 0 | 56,159,404 |
| Combined strategy vs bot bot 3, native seed 402, same maps / colors | 20 | 8–12 | 0 | Unmetered |
| Exact final package vs bot bot 3, sandbox seed 404, same maps / colors | 20 | 10–10 | 0 | 56,416,128 |
| Exact final package self-play, Portals / Schooltime, seed 405 / both colors / twice | 8 runs, 4 matchups | Every repeated replay hash identical | 0 | 44,653,650 |

The sandbox gate requires candidate turns below 90 million points; the competition
limit is 100 million. There were no candidate runtime errors, no-valid-action
deaths, or disallowed avoidable visible collisions in these completed runs.
Intentional trades were independently checked using live funding and visible
enemy heads. Opponent failures remain recorded: one in the native diagnostic,
zero in either all-map sandbox comparison. The interrupted seed 403 self-play
trial used superseded sonar code and is excluded from validation totals.

The earlier combined strategy precedes the final sonar team-separation refinement.
Only the seed 404 and 405 checks certify the exact final source fingerprint.

## Exact final package results by map

The queen columns show candidate / opponent final fixed-queen lengths in colors
A and B respectively. A dead queen is zero; secondary scores decide ties.

| Map | A | B | Queen lengths, A game | Queen lengths, B game |
| --- | --- | --- | --- | --- |
| Autarky | Win | Loss | 0 / 0 | 0 / 0 |
| Default | Win | Loss | 15 / 0 | 0 / 20 |
| Devil | Loss | Loss | 0 / 0 | 0 / 0 |
| Portals | Loss | Win | 3 / 5 | 5 / 3 |
| Prisoners Dilemma | Loss | Win | 0 / 0 | 0 / 0 |
| Queen Of Spades | Win | Win | 17 / 0 | 26 / 7 |
| Schooltime | Loss | Loss | 0 / 0 | 0 / 0 |
| Slithery Fight | Win | Loss | 0 / 0 | 0 / 0 |
| Trauma | Loss | Win | 0 / 17 | 18 / 0 |
| Trophy | Win | Win | 0 / 0 | 0 / 0 |

Replay-event metrics from those 20 exact-package games:

| Metric | Candidate | Bot bot 3 |
| --- | ---: | ---: |
| Turns | 104,568 | 125,073 |
| Pearls collected | 5,896 | 6,325 |
| Pearls per turn | 0.05638 | 0.05057 |
| Successful portal crossings | 40 | 26 |
| Queens alive at finish | 6 | 5 |
| Sum of final queen lengths | 84 | 52 |

Food per turn is about 11.5% higher in this sample; total food is lower because
the candidate takes fewer turns. Portal use increased, but counts do not prove
that a crossing helped win. Across the two sandbox strategy seeds the outcome
was 22–18; the independent native seed was 8–12. These samples do not establish
statistically reliable superiority or a competition ranking. Opponent source
from the live competition is unavailable. Forced traps, body congestion and
queen deaths remain material strategic weaknesses even when actions are legal.

## Package identity and saved evidence

Source commit: `cd4c889` (the subsequent documentation commit does not alter bot
sources). All 32 packaged files match the working sources byte for byte; tests,
libraries, utilities, replay data and credentials are excluded. The ZIP passes
integrity checks and is reproducible from the stable profile.

- ZIP SHA256: `760915bac337a593d32096b2b22fc66f0cd3abc8df7ed82b4df3d8a60def730f`
- Packaging source SHA256: `f5c3b14edfd49859d0da70d190234059bc4b6b81e3cb29e551811aafbc6d303c`
- Judge selected-source fingerprint: `f619db18e68cddbf3c84afc722220057b77cd3f3153dad072002baf65e106d33`

Local artifacts are saved under `build/validation/final-sandbox20/` and
`build/validation/final-self-repeat8/`: summaries, source/map/engine fingerprints,
matches JSONL, death diagnostics and replays. Final gameplay aggregates and
per-game metrics are in `final-sandbox20/replay-metrics.json`. Intermediate
comparisons and frozen sources remain under `build/validation/` and `build/`.
The implementation history is recorded in `NEXT_VERSION_PROGRESS.md`.

Reproduce the final comparison using the pinned environment:

```bash
XDG_CACHE_HOME=/tmp/sudo-win-live-cache /tmp/sudo-win-live-venv/bin/python \
  algo_bot/util/benchmark.py build/submission-competition-v5 \
  build/submission-replay-v4-conservative \
  --maps build/competition-maps/{19,4,13,20,17,7,9,21,15,11}.map \
  --seeds 404 --both-colours --allow-favourable-trades \
  --output build/validation/reproduce-v5-fresh
```

Use a fresh output directory so earlier evidence is preserved.

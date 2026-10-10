# Bot bot 3 improvement progress

Baseline: submission 14545, local commit bc56143, source package
`build/submission-replay-v4-conservative.zip`. Replay snapshot: 70 games,
21 wins / 49 losses; 29 eliminations and 20 round-limit losses. Twenty-eight
elimination losses ended in head collisions; our fixed queen died in 48 losses.
The full review is saved in `REPLAY_BOT3_REVIEW.md`.

## Implementation and verification plan

Each step receives its own `aman/feat:` commit after targeted checks. Strategy
experiments are evaluated individually before the combined candidate is packaged.
No upload or live challenges are part of this task.

| Step | Work | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Current official judge, fixed queen scoring, free sprint accounting | Complete | 45 C++ cases / 289 assertions, sanitizers, 16 Python tests; native 2–2 / four games, zero candidate errors |
| 2 | Queen identity, food priority and parent-preserving rescue | Complete | 45 C++ cases / 299 assertions; native 4–4 / eight games, zero candidate errors |
| 3 | Bounded free sprint search, opening escapes and turn-order-aware threats | Complete | 48 C++ cases / 319 assertions; ordered-threat native comparison 3–5, zero errors |
| 4 | Earlier remembered portal escapes and helper exploration | Complete | 49 C++ cases / 326 assertions; native 5–3 against step 3b, zero errors |
| 5 | Useful sonar reports and helper attacks against enemy queen | Complete | Native 3–5; sandbox 1–1, no errors, peak 43,533,506 CPU points |
| 6 | Resource-supported helper expansion and endgame coordination | Complete | 54 C++ cases / 356 assertions; native 4–4, no errors; four compiler/config checks pass |
| 7 | Combined held-out sandbox checks, repeatability and final package | Complete | Exact package: 20 sandbox games, 10–10, zero errors; 8 repeated self-play runs deterministic; peak 56,416,128 |

## Rules that validation must enforce

- At the round limit: fixed queen length, then longest living snake, then total
  living length. A dead queen scores zero. Queen IDs are 0 and 1, but either team
  can own either ID. https://game.battlecode.au/docs/structure
- The first `ceil(action_start_length / 4)` moves are free; later steps require
  length above two before moving and remove an additional tail segment after
  collision/growth checks. Pearl growth does not change the free allowance.
  https://game.battlecode.au/docs/movement
- Collision checks occur before movement, including when entering one's own tail.
- Split children get a new program instance and act later in the same round.
- Sonar echoes are delayed aggregate counts, not a map or proof of empty exits.

## Progress notes

- Started the requested implementation. Repository was clean at baseline.
- Earlier local comparisons used an outdated judge and cannot establish live-rule
  superiority. Correct rule conformance comes before tuning strategy.

### Step 1 — live rules

Pinned unswbc 1.2.5 and engine SHA256 `26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546`. Public repository main still contains the older engine; the current official wheel and live-rule probes are the validation authority. Free allowance stays fixed at action-start length, and body predictions, combat funding and the independent collision checker use it. A replay-derived WSS fixture catches retained-tail collisions. Benchmarks report failures for both teams but promotion gates the candidate, so a known-buggy old opponent cannot mask candidate correctness. Native diagnostic evidence: `build/validation/live-rules-native4/` (Autarky / Portals, seed 301, both colors, 2–2, zero errors; no CPU measurement).

### Step 2 — fixed queen policy

IDs 0 and 1 keep the queen role regardless of color, relative visible lengths or unit count. Helpers defer legal resource claims to a closer queen; queens never yield to helpers or invest in population splits. A certified queen rescue needs a bounded parent escape and no direct attack on the stationary parent; last-resort splits remain legal fallbacks when every move is fatal. Free zero-cost movement is permitted for protected units. Release and ASan/UBSan checks pass. Diagnostic comparison against step 1: `build/validation/queen-native8/`, seed 302, both colors on Autarky, Prisoners Dilemma, Portals and Trauma: 4–4, no candidate errors. This establishes integration correctness, not win-rate superiority. Longer opening escapes are addressed in step 3.

### Step 3a — bounded free movement and forced queen fallback

Implemented an eight-step hard cap, 256 expansion-node budget and two beam candidates per first direction. The action-start free allowance bounds long routes; every step is simulated with live collision/tail rules. A length-17 queen collects five pearls in five free steps in the regression fixture. Some competition maps deliberately force the starting queen into a cul-de-sac (Autarky / Prisoners Dilemma / Slithery Fight); when all ordinary moves are fatal and parent rescue cannot be certified, a legal split preserves a larger secondary contender rather than repeatedly draining the queen into tiny children. No safety guarantee is attached to this last-resort child.

46 C++ cases / 308 assertions pass in Release and ASan/UBSan. Diagnostic comparison against step 2: native seed 303, four maps / both colors, 4–4 (`build/validation/free-sprint-native8/`). Held-out sandbox seed 304 on Slithery Fight / Portals / both colors: 1–3, zero candidate errors, peak 43,490,830 / 100,000,000 (`build/validation/free-sprint-sandbox4/`). CPU integration is verified; win-rate advantage is not established by this small sample. Broader combined comparisons will determine final strategy settings. Turn-order-aware defense remains in progress.

### Step 3b — funded attacks and turn order

Threat maps now keep separate funded earlier/later enemy routes, including the live free allowance and intermediate pearl income. Queens and last survivors rank endpoints reachable by later funded attacks within three steps below unattacked endpoints. Helpers retain softer risk costs. Stationary split parents and reversed child heads are checked against later funded attacks before rescue certification. Release and ASan/UBSan pass 48 cases / 319 assertions. Native seed 305 on Default, Trophy, Queen of Spades and Devil / both colors: 3–5 against step 3a, no candidate or opponent errors (`build/validation/ordered-threat-native8/`). Defense is integrated; this small diagnostic does not establish a win-rate improvement.

### Step 4 — helper portals

Small nonqueen helpers (length at most four, more than one survivor, not the secondary champion) can take a single uncertain portal step after eight rounds without growth and no resource route, or when local movement is about to fail. Fresh remembered exits are preferred; unknown pairs can be sampled, but known blocked/stale exits are never relabelled as unknown. Twelve-round per-helper cooldown prevents immediate repeated probing. Nonqueens may also escape through a validated remembered exit before their last visible move fails; queens keep the original escape-only gate and certified rescue priority. Complete body knowledge is required. Release and ASan/UBSan pass 49 cases / 326 assertions. Native seed 306 on Portals, Autarky, Prisoners Dilemma and Schooltime / both colors: 5–3, zero errors (`build/validation/portals-native8/`). This is promising diagnostic evidence, with full sandbox validation still required.

### Step 5 — bounded reports and queen targeting

Sonar now sends at most two directed 64-bit beams per action, rotating queen status, legal resource claims, canonical portal endpoints and observed food/enemy-queen sightings. The old eight-bit sender field is replaced with thirteen bits; encoding rejects overflow and decoding reconstructs delayed absolute rounds. At most 64 fresh reports are retained. Reports influence helper roles, resource allocation and soft enemy-queen approach/caution; portal reports extend static pairing only when they do not contradict directly seen edges. They never mark remote tiles seen or override occupancy. The payload tag filters unrelated messages; it is not cryptographic authentication and remote information remains advisory.

Length-two/three helpers with a surviving teammate can deliberately remove a currently visible enemy queen along a fully simulated, funded route. Queens, last survivors and incomplete bodies cannot be sacrificed. Broader length trades remain disabled by default. The independent benchmark classifier explicitly verifies queen victims and rejects sacrificing our queen; intentional head trades require the benchmark opt-in. Release and ASan/UBSan pass 54 cases / 352 assertions; 16 Python tests pass. Frozen source: `build/live-step5-sonar`. Native seed 307 eight-game comparison and sandbox seed 308 two-map CPU checks are running; six native games completed without errors at commit time. Results will be recorded before final promotion.

Completed step 5 comparisons: native seed 307, Portals / Queen of Spades / Trophy / Schooltime, both colors: 3–5; sandbox seed 308, Portals / Schooltime: 1–1, zero errors, candidate peak 43,533,506 (`build/validation/sonar-native8/`, `sonar-sandbox2/`). No broad win-rate advantage is inferred.

### Step 6 — coordinated population and late scoring

Enabled early expansion into a two-segment helper only for nonqueens with independent food routes and multiple six-turn escapes for both resulting snakes. Bounded body simulation replaces geometric distance when allocating that income; walls, stationary split bodies and competing queen claims can invalidate it. Investment stops at eight living units and round 280; the secondary champion is preserved once there are teammates. Fixed queen identity is enforced even if a caller mislabels its role. Late protected snakes keep the paid zero-growth sprint restriction; helper queen removal remains available because it changes the primary score. Ally estimates are now expired rather than accumulated indefinitely. Aggregate enemy-head sonar contact can defer blind probing, without locating an enemy or claiming an exit is blocked/empty.

Stable packaging preserves these promoted feature settings. Each ablation profile changes exactly one flag, and CI explicitly verifies allowed funded helper head trades. Release (Apple Clang, LLVM Clang and GCC), ASan/UBSan and 16 Python tests pass; 54 C++ cases / 356 assertions. Native seed 309 on Default / Schooltime / Trauma / Slithery Fight, both colors: 4–4, zero errors (`build/validation/growth-native8/`). Frozen combined source: `build/live-step6-growth`; broad held-out checks remain in progress.

### Final integration — team separation

Sonar does not supply sender-team metadata. Integration review found that a mirrored opponent could emit our format, so the wire tag now includes an explicit team bit and a team-dependent checksum. Opposite-team copies cannot accidentally accept each other's reports; this still does not make the format cryptographic authentication. Bot encode/decode uses the current controller's team, independent of queen ID. Regression tests cover both colors and cross-team rejection. Release and ASan/UBSan pass 55 cases / 362 assertions.

The first self-repeat trial (`combined-self-repeat8`, seed 403) was intentionally interrupted after one completed game when this refinement superseded its source; it is excluded from completed validation evidence. Earlier combined seed 401 sandbox / seed 402 native trials remain strategy diagnostics. The exact final package is `build/submission-competition-v5.zip`, with all 32 selected files identical to working sources.

### Step 7 — completed combined validation

All four compiler/configuration suites pass 55 C++ cases / 362 assertions; 16 Python tests and independent official-engine live-rule probes pass. Exact package seed 404 across all ten competition maps / both colors: 10–10, zero candidate or opponent errors, peak 56,416,128 / 100,000,000 CPU points. Seed 405 self-play on Portals and Schooltime / both colors / two repetitions: eight runs with matching replay hashes, zero errors, peak 44,653,650. Earlier seed 401 sandbox strategy comparison was 12–8; seed 402 native diagnostic was 8–12 (zero candidate errors, one opponent error). These small samples establish correctness and runtime integration, not a reliable competition win-rate advantage.

Exact final replay metrics: 5,896 pearls / 104,568 turns versus 6,325 / 125,073 (about 11.5% better food per turn), 40 versus 26 portal crossings, six versus five queens alive, total final queen lengths 84 versus 52. Map/seed variance and forced traps remain limitations. Full evidence, per-map results, package hashes and reproduction command: `LIVE_V5_VALIDATION.md`. No upload or live activation was performed. Every requested implementation step is saved independently.

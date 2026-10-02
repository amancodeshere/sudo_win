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
| 3 | Bounded free sprint search, opening escapes and turn-order-aware threats | In progress | Free movement is underused; two-step attacks dominate elimination |
| 4 | Earlier remembered portal escapes and helper exploration | Pending | Only 16 successful crossings in 70 games |
| 5 | Useful sonar reports and helper attacks against enemy queen | Pending | Existing messages are decoded but unused |
| 6 | Resource-supported helper expansion and endgame coordination | Pending | Repeated rescue cycles and weak food collection |
| 7 | Combined held-out sandbox checks, repeatability and final package | Pending | Validate actual queen scoring, live payment rules and CPU |

## Rules that validation must enforce

- At the round limit: fixed queen length, then longest living snake, then total
  living length. A dead queen scores zero. Queen IDs are 0 and 1, but either team
  can own either ID. https://game.battlecode.au/docs/structure
- The first `ceil(action_start_length / 4)` moves are free; later moves cost one
  segment before moving. Pearl growth does not change the action's free allowance.
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

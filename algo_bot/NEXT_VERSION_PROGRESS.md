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
| 1 | Current official judge, fixed queen scoring, free sprint accounting | In progress | Official docs differ from cached engine and simulation |
| 2 | Queen identity, food priority, parent-preserving rescue and opening escapes | Pending | Queen often dies after repeated splits |
| 3 | Bounded free sprint search and turn-order-aware threats | Pending | Free movement is underused; two-step attacks dominate elimination |
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

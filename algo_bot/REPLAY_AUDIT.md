# Competition replay audit

All 75 downloaded games were inspected event by event. The submitted archive (version 14151) matches the stable source archive byte for byte. The replay body reconstruction agrees with the recorded final count, longest length, and total length for both teams in every game.

Results: **24 wins, 51 losses**. Of the losses, 42 ended by elimination and 9 at the round limit with insufficient length. Our 228 deaths comprise 130 self collisions, 74 head-to-head collisions, and 24 collisions with another body. No invalid-action deaths or timeouts were recorded; maximum measured CPU use was 48,628,124 points.

61 deaths happened in the first ten rounds. Slithery Fight starts one snake with no legal movement; always moving guarantees its loss. Long snakes also become trapped on Autarky and Prisoners Dilemma. Last-turn movement fixes cannot recover these states; rescue splits and earlier trap detection are needed. Opponent populations often grow to dozens through splitting, while our version never splits.

The strongest opportunities are legal rescue splits, better growth-versus-space planning, longer enemy threat horizons, and population investment that protects the longest snake. Round-limit scoring uses the longest surviving snake, then total length; reckless population growth is not itself a winning condition.

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

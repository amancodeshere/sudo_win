#!/usr/bin/env python3
"""Summarize bounded decision diagnostics emitted by the diagnostic profile."""
import argparse
from collections import Counter
import json
from pathlib import Path
from analyze_replays import Board, SCHEMA


def measure(path: Path, candidate: str) -> dict:
    raw = path.read_bytes()
    replay = SCHEMA.Replay.from_bytes_packed(raw, traversal_limit_in_words=max(1000000,len(raw)*64))
    if replay.formatVersion > 2:
        raise ValueError('unsupported replay format')
    board = Board(replay.map)
    teams = {i:d['team'] for i,d in board.dragons.items()}
    reasons, early_growth = Counter(), Counter()
    round_num = -1
    for event in replay.events:
        kind = event.which()
        e = getattr(event,kind)
        if kind == 'roundStart':
            round_num = e.round
        elif kind == 'dragonSplit':
            teams[e.childId] = str(e.team).upper()
        elif kind == 'dragonIndicator' and teams[e.id] == candidate:
            reason, separator, growth = e.text.partition('; growth: ')
            reasons[reason] += 1
            if separator and round_num < 100:
                early_growth[growth] += 1
    return {'map':board.name,'reasons':dict(reasons),'early_growth':dict(early_growth)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder',type=Path)
    args = parser.parse_args()
    rows = [json.loads(line) for line in (args.folder/'matches.jsonl').read_text().splitlines()]
    games = [measure(Path(r['replay']),r['candidate_team']) | {'seed':r['seed'],'candidate_team':r['candidate_team']} for r in rows]
    summary = {'games':games,'reasons':dict(sum((Counter(g['reasons']) for g in games),Counter())),
               'early_growth':dict(sum((Counter(g['early_growth']) for g in games),Counter()))}
    (args.folder/'decision-metrics.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps({k:v for k,v in summary.items() if k!='games'},indent=2))


if __name__ == '__main__':
    main()

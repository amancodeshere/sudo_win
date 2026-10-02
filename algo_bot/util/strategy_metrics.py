#!/usr/bin/env python3
"""Measure scoring bodies, early population and movement cost in benchmark replays."""
from __future__ import annotations
import argparse
from collections import Counter
import json
from pathlib import Path
from statistics import median
from analyze_replays import Board, DIRECTIONS, NAMES, SCHEMA


def measure(path: Path, planned_donors: set[int] | None = None) -> dict:
    raw = path.read_bytes()
    replay = SCHEMA.Replay.from_bytes_packed(raw, traversal_limit_in_words=max(1000000, len(raw) * 64))
    if replay.formatVersion > 2:
        raise ValueError('unsupported replay format')
    board = Board(replay.map)
    queens = {t: min(i for i, d in board.dragons.items() if d['team'] == t) for t in 'AB'}
    stats = {t: Counter() for t in 'AB'}
    planned_donors = planned_donors or set()
    food_origin = {}
    death_round = {}
    teams = {i: d['team'] for i, d in board.dragons.items()}
    lives = {i: {'turns': 0, 'child': False} for i in teams}
    actor, round_num, action_steps, directions = None, -1, 0, []
    population = {t: {} for t in 'AB'}
    queen_deaths = {}
    for event in replay.events:
        kind = event.which()
        e = getattr(event, kind)
        if kind == 'roundStart':
            if round_num in (25, 100):
                for t in 'AB':
                    population[t][str(round_num)] = sum(d['team'] == t for d in board.dragons.values())
            round_num = e.round
        elif kind == 'turnStart':
            actor, action_steps, directions = e.id, 0, []
            stats[teams[actor]]['turns'] += 1
            lives[actor]['turns'] += 1
        elif kind == 'dragonAction':
            action = e.action.to_dict()
            directions = action.get('move', [])
            length = len(board.dragons[e.id]['body'])
            stats[teams[e.id]]['declared_paid_steps'] += max(0, len(directions) - (length + 3) // 4)
        elif kind == 'tileChange':
            p = (e.tile.x, e.tile.y)
            if e.hasPearl:
                board.pearls.add(p)
            else:
                if p in board.pearls and actor in board.dragons:
                    t = teams[actor]
                    stats[t]['pearls'] += 1
                    stats[t]['queen_pearls'] += actor == queens[t]
                    origin = food_origin.pop(p, None)
                    stats[t]['queen_planned_donor_pearls'] += actor == queens[t] and origin in planned_donors
                    stats[t]['queen_planned_donor_pearls_within_4'] += (actor == queens[t]
                        and origin in planned_donors and round_num - death_round[origin] <= 4)
                board.pearls.discard(p)
        elif kind == 'dragonUpdate':
            d = board.dragons[e.id]
            d['facing'] = DIRECTIONS[NAMES.index(str(e.facing))]
            if round_num >= 0:
                boundary = board.boundary(d['body'][0], d['facing'])
                stats[d['team']]['portal_crossings'] += board.edges[boundary] not in ('.', 'w')
                d['body'].insert(0, (e.head.x, e.head.y))
                while len(d['body']) > 1 and d['body'][-1] != (e.tail.x, e.tail.y):
                    d['body'].pop()
                if actor == e.id:
                    action_steps += 1
        elif kind == 'dragonSplit':
            t = str(e.team).upper()
            teams[e.childId] = t
            lives[e.childId] = {'turns': 0, 'child': True}
            board.dragons[e.parentId]['body'] = [(p.x, p.y) for p in e.parentBody]
            board.dragons[e.childId] = {'team': t, 'body': [(p.x, p.y) for p in e.childBody],
                                      'facing': DIRECTIONS[NAMES.index(str(e.childFacing))]}
            stats[t]['children_created'] += 1
        elif kind == 'dragonDeath':
            d = board.dragons[e.id]
            t = d['team']
            death_round[e.id] = round_num
            stats[t]['planned_donations'] += e.id in planned_donors
            for p in d['body'][::2]:
                if p not in board.pearls:
                    food_origin[p] = e.id
            if e.id == queens[t]:
                blocker = str(e.reason)
                if actor == e.id and action_steps < len(directions):
                    target = board.step(d['body'][0], DIRECTIONS[NAMES.index(directions[action_steps])])
                    occupant = board.occupancy().get(target)
                    if occupant is not None:
                        blocker = 'self' if occupant == e.id else 'ally' if teams[occupant] == t else 'enemy'
                queen_deaths[t] = {'round': round_num, 'reason': str(e.reason), 'blocker': blocker,
                                   'length': len(d['body'])}
            stats[t]['children_dead_within_5_turns'] += lives[e.id]['child'] and lives[e.id]['turns'] <= 5
            board.dragons.pop(e.id)
    for t, field in [('A', 'teamA'), ('B', 'teamB')]:
        bodies = [d['body'] for d in board.dragons.values() if d['team'] == t]
        actual = getattr(replay.result, field)
        if (actual.dragonCount, actual.totalLength, actual.longestDragon) != (
                len(bodies), sum(map(len, bodies)), max(map(len, bodies), default=0)):
            raise ValueError(f'replay reconstruction disagrees with engine: {path}')
        stats[t]['final_queen'] = len(board.dragons.get(queens[t], {}).get('body', []))
        stats[t]['final_longest'] = actual.longestDragon
    return {'map': board.name, 'stats': {t: dict(s) for t, s in stats.items()},
            'population': population, 'queen_deaths': queen_deaths}


def summarize(folder: Path) -> dict:
    games = []
    for line in (folder / 'matches.jsonl').read_text().splitlines():
        row = json.loads(line)
        r = measure(Path(row['replay']), {d['id'] for d in row['deaths'] if d.get('verified_donation')})
        games.append(r | {'candidate_team': row['candidate_team'], 'seed': row['seed'], 'winner': row['winner']})
    summary = {}
    for side in ('candidate', 'opponent'):
        pairs = [(r, r['candidate_team'] if side == 'candidate' else ('B' if r['candidate_team'] == 'A' else 'A')) for r in games]
        totals = sum((Counter(r['stats'][t]) for r, t in pairs), Counter())
        summary[side] = {'totals': dict(totals), 'queens_survived': sum(r['stats'][t]['final_queen'] > 0 for r, t in pairs),
                         'queen_blockers': dict(Counter(r['queen_deaths'][t]['blocker'] for r, t in pairs if t in r['queen_deaths'])),
                         'median_population': {str(n): median(values) if (values := [r['population'][t][str(n)] for r, t in pairs if str(n) in r['population'][t]]) else None for n in (25, 100)}}
    result = {'games': games, 'summary': summary}
    (folder / 'strategy-metrics.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', type=Path)
    args = parser.parse_args()
    print(json.dumps(summarize(args.folder)['summary'], indent=2))

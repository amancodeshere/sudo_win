#!/usr/bin/env python3
"""Audit competition replay events and export observed turns for regression tests."""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import gzip
import hashlib
import json
from pathlib import Path
import capnp

DIRECTIONS = "NESW"
DELTAS = [(0, -1), (1, 0), (0, 1), (-1, 0)]
NAMES = ["north", "east", "south", "west"]
SCHEMA = capnp.load(str(Path(__file__).with_name("replay.capnp")))


def point(p):
    return (p["x"], p["y"])


class Board:
    def __init__(self, text):
        self.dragons, self.edges, self.pearls, self.countdown, self.portals = {}, {}, set(), {}, defaultdict(set)
        self.name = "unknown"
        for line in text.splitlines():
            f = line.split()
            if not f: continue
            if f[0] == "MAP": self.width, self.height = map(int, f[1:])
            elif f[0] == "MAP_NAME": self.name = " ".join(f[1:])
            elif f[0] == "TILE":
                x, y, min_gap, max_gap = map(int, f[1:])
                if max_gap > 0: self.countdown[(x, y)] = 0
            elif f[0] == "EDGE":
                edge, kind, portal = map(int, f[1:])
                row, x = divmod(edge, self.width + 1)
                # The judge ignores right/bottom padding boundaries.
                if x == self.width or row == 2*self.height: continue
                key = ("H" if row % 2 == 0 else "V", x % self.width, (row // 2) % self.height)
                self.edges[key] = "." if kind == 0 else "w" if kind == 1 else str(portal)
                if kind == 2: self.portals[str(portal)].add(key)
            elif f[0] in ("DRAGON", "SNAKE"):
                team, length, *coords = map(int, f[1:])
                body = list(zip(coords[::2], coords[1::2]))
                assert len(body) == length
                self.dragons[len(self.dragons)] = {"team": "AB"[team], "body": body, "facing": "N"}
        for d in self.dragons.values():
            d["facing"] = self.direction(d["body"][1], d["body"][0]) if len(d["body"]) > 1 else "E"

    def boundary(self, p, direction):
        x, y = p
        return {"N": ("H", x, y), "S": ("H", x, (y+1) % self.height),
                "W": ("V", x, y), "E": ("V", (x+1) % self.width, y)}[direction]

    def step(self, p, direction):
        key = self.boundary(p, direction)
        token = self.edges.get(key, "w")
        if token == "w": return None
        if token == ".":
            dx, dy = DELTAS[DIRECTIONS.index(direction)]
            return ((p[0]+dx) % self.width, (p[1]+dy) % self.height)
        ends = self.portals[token]
        if len(ends) != 2: return None
        partner = next(e for e in ends if e != key)
        _, x, y = partner
        return ((x - (direction == "W")) % self.width, (y - (direction == "N")) % self.height)

    def direction(self, tail, head):
        return next(d for d in DIRECTIONS if self.step(tail, d) == head)

    def visible(self, origin, p):
        dx, dy = (p[0]-origin[0]) % self.width, (p[1]-origin[1]) % self.height
        return (dx <= 3 or dx >= self.width-3) and (dy <= 3 or dy >= self.height-3)

    def occupancy(self):
        return {p: i for i, d in self.dragons.items() for p in d["body"]}

    def alternatives(self, identity):
        head = self.dragons[identity]["body"][0]
        occupied = self.occupancy()
        return [d for d in DIRECTIONS if (p := self.step(head, d)) is not None
                and p not in occupied and self.visible(head, p)]

    def block(self, identity, round_num):
        d = self.dragons[identity]
        head = d["body"][0]
        out = [f"ROUND {round_num}", f"DIR {d['facing']}", f"LENGTH {len(d['body'])}",
               f"UNIT_COUNT {sum(x['team'] == d['team'] for x in self.dragons.values())}", "NUM_MSGS 0"]
        for row in range(7):
            for column in range(7):
                p = ((head[0]+column-3) % self.width, (head[1]+row-3) % self.height)
                out.append(f"{p[0]} {p[1]} {int(p in self.pearls)} {self.countdown.get(p, -1)}")
        parts = []
        for other, dragon in self.dragons.items():
            for rank, p in enumerate(dragon["body"]):
                if self.visible(head, p):
                    facing = dragon["facing"] if rank == 0 else self.direction(p, dragon["body"][rank-1])
                    parts.append(f"{dragon['team']} {other} {p[0]} {p[1]} {facing} {int(rank == 0)}")
        out += [f"DRAGON_BODIES {len(parts)}", *parts]
        for kind, rows, columns in (("H", 8, 7), ("V", 7, 8)):
            for row in range(rows):
                out.append(" ".join(self.edges[(kind, (head[0]+column-3) % self.width,
                                                (head[1]+row-3) % self.height)] for column in range(columns)))
        return "\n".join(out) + "\n"


def analyze(path, metadata, output):
    raw = path.read_bytes()
    replay = SCHEMA.Replay.from_bytes_packed(raw, traversal_limit_in_words=max(1000000, len(raw)*64))
    if replay.formatVersion > 2: raise ValueError("Unsupported replay format")
    board = Board(replay.map)
    ours = metadata['ours']
    stats = {t: {"turns": 0, "moves": 0, "sprints": 0, "sprint_payments": 0, "splits": 0,
                 "pearls": 0, "max_points": 0, "timeouts": 0, "peak_length": 0} for t in 'AB'}
    deaths, last_turn, traces, logs = [], {}, [], []
    round_num, actor = -1, None
    for event in replay.events:
        kind = event.which()
        e = getattr(event, kind).to_dict()
        if kind == 'roundStart':
            round_num = e['round']
            for p in board.countdown: board.countdown[p] = max(0, board.countdown[p] - 1)
        elif kind == 'pearlCountdown': board.countdown[point(e['tile'])] = e['countdown']
        elif kind == 'tileChange':
            p = point(e['tile'])
            if e['hasPearl']: board.pearls.add(p)
            else:
                if actor in board.dragons: stats[board.dragons[actor]['team']]['pearls'] += 1
                board.pearls.discard(p)
        elif kind == 'turnStart':
            actor = e['id']
            d = board.dragons[actor]
            stats[d['team']]['turns'] += 1
            record = {"id": actor, "round": round_num, "length": len(d['body']), "head": d['body'][0],
                      "safe_directions": board.alternatives(actor), "nearby_enemy_heads": [
                          {"id": i, "head": x['body'][0], "length": len(x['body'])}
                          for i, x in board.dragons.items() if x['team'] != d['team'] and board.visible(d['body'][0],x['body'][0])]}
            if d['team'] == ours:
                record['init'] = f"ID {actor}\nTEAM {ours}\nMAP {board.width} {board.height}\nUNIT_LIMIT 64\n"
                record['stdin'] = board.block(actor, round_num)
                traces.append(record)
            last_turn[actor] = record
        elif kind == 'dragonAction':
            identity = e['id']
            team = board.dragons[identity]['team']
            action = e.get('action', {})
            steps = action.get('move', [])
            stats[team]['moves'] += bool(steps)
            stats[team]['sprints'] += len(steps) > 1
            stats[team]['sprint_payments'] += max(0, len(steps)-1)
            stats[team]['splits'] += 'split' in action
            stats[team]['max_points'] = max(stats[team]['max_points'], e.get('instructions',{}).get('count',0))
            stats[team]['timeouts'] += e.get('tle', False)
            if identity in last_turn:
                last_turn[identity]['action'] = action
                last_turn[identity]['points'] = e.get('instructions',{}).get('count')
        elif kind == 'dragonUpdate':
            identity = e['id']
            d = board.dragons[identity]
            d['facing'] = DIRECTIONS[NAMES.index(e['facing'])]
            if round_num >= 0:
                d['body'].insert(0,point(e['head']))
                while len(d['body']) > 1 and d['body'][-1] != point(e['tail']): d['body'].pop()
            stats[d['team']]['peak_length'] = max(stats[d['team']]['peak_length'],len(d['body']))
        elif kind == 'dragonSplit':
            board.dragons[e['parentId']]['body'] = [point(p) for p in e['parentBody']]
            board.dragons[e['childId']] = {'team':e['team'].upper(), 'body':[point(p) for p in e['childBody']],
                                          'facing':DIRECTIONS[NAMES.index(e['childFacing'])]}
        elif kind == 'dragonDeath':
            identity = e['id']
            d = board.dragons.pop(identity)
            deaths.append({"id":identity,"team":d['team'],"round":round_num,"reason":e['reason'],
                           "length":len(d['body']),"last_turn":last_turn.get(identity)})
        elif kind in ('engineLog','dragonLog') and board.dragons.get(e['id'],{}).get('team') == ours:
            logs.append({'round':round_num, **e})
    result = replay.result.to_dict()
    for team, field in (('A','teamA'),('B','teamB')):
        bodies = [d['body'] for d in board.dragons.values() if d['team'] == team]
        assert result[field]['dragonCount'] == len(bodies), path
        assert result[field]['totalLength'] == sum(map(len,bodies)), path
        assert result[field]['longestDragon'] == max(map(len,bodies),default=0), path
    winner = result.get('winner','').upper()
    own_deaths = [d for d in deaths if d['team'] == ours]
    record = {"game":int(path.stem), **metadata, "map":board.name,"rounds":round_num+1,
              "outcome":"draw" if not winner else "win" if winner == ours else "loss",
              "result":result,"stats":stats,"deaths":deaths,"logs":logs,
              "replay_sha256":hashlib.sha256(raw).hexdigest(),"bot":replay.botA if ours=='A' else replay.botB}
    record['loss_class'] = ('none' if record['outcome']=='win' else 'draw' if not winner else
                           'growth deficit' if result['endReason']=='roundLimit' else 'eliminated')
    record['our_death_reasons'] = dict(Counter(d['reason'] for d in own_deaths))
    with gzip.open(output / f"{path.stem}.turns.jsonl.gz",'wt') as f:
        for t in traces: f.write(json.dumps(t)+'\n')
    (output / f"{path.stem}.json").write_text(json.dumps(record,indent=2)+'\n')
    (output / f"{path.stem}.map").write_text(replay.map)
    return record


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--replays',type=Path,default=Path('replays'))
    p.add_argument('--team-name',default='sudo win')
    p.add_argument('--output',type=Path,default=Path('build/replay-audit'))
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    metadata={}
    for f in args.replays.glob('battle-*.json'):
        detail=json.loads(f.read_text())
        ours='A' if detail['teamAName']==args.team_name else 'B' if detail['teamBName']==args.team_name else None
        if ours is None: continue
        opponent=detail['teamBName'] if ours=='A' else detail['teamAName']
        for game in detail['games']: metadata[game['id']]={'ours':ours,'opponent':opponent,'battle':detail['match']['id']}
    records=[]
    for f in sorted(args.replays.glob('*.replay')):
        if int(f.stem) not in metadata: raise ValueError(f'Missing team metadata for {f}')
        r=analyze(f,metadata[int(f.stem)],args.output);records.append(r)
        print(f"{f.stem} {r['map']} {r['outcome']} {r['loss_class']} {r['our_death_reasons']}",flush=True)
    summary={'games':len(records),'outcomes':dict(Counter(r['outcome'] for r in records)),
             'loss_classes':dict(Counter(r['loss_class'] for r in records if r['outcome']=='loss')),
             'our_death_reasons':dict(sum((Counter(r['our_death_reasons']) for r in records),Counter())),
             'max_points':max(r['stats'][r['ours']]['max_points'] for r in records)}
    (args.output/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    (args.output/'index.json').write_text(json.dumps([{k:v for k,v in r.items() if k not in ('deaths','logs')} for r in records],indent=2)+'\n')
    print(json.dumps(summary,indent=2))
    return 0

if __name__=='__main__': raise SystemExit(main())

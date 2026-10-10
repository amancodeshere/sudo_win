#!/usr/bin/env python3
"""Check two native bots for identical actions on exported replay observations.

This verifies behavior equivalence, not counterfactual wins: the later inputs
remain those recorded in the original match even if a tested bot would diverge.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
from unswbc.bot import Bot, Pool
from benchmark import action_metrics


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before', type=Path)
    parser.add_argument('after', type=Path)
    parser.add_argument('--audit', type=Path, default=Path('build/replay-audit'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    inputs = sorted(args.audit.glob('*.turns.jsonl.gz'))
    if not inputs:
        parser.error('Run analyze_replays.py first to export turn observations')
    binaries = [args.before.resolve(), args.after.resolve()]
    pools = [Pool([str(binary)], size=0) for binary in binaries]
    records = []
    try:
        for path in inputs:
            workers = [{}, {}]
            record = {'game': int(path.name.split('.')[0]), 'turns': 0,
                      'differences': 0, 'errors': [], 'examples': []}
            try:
                with gzip.open(path, 'rt') as stream:
                    for line in stream:
                        turn = json.loads(line)
                        identity = turn['id']
                        outputs = []
                        for index, pool in enumerate(pools):
                            if identity not in workers[index]:
                                workers[index][identity] = Bot(pool, init=turn['init'].encode(), name=str(identity))
                            bot = workers[index][identity]
                            output = bot.ask(turn['stdin'].encode())
                            _, error = action_metrics(output)
                            if error or bot.error:
                                record['errors'].append({'round': turn['round'], 'id': identity,
                                                         'bot': index, 'error': error or bot.error})
                            outputs.append([s for s in output.decode().splitlines()
                                            if s.startswith(('MOVE ', 'SPLIT '))])
                        record['turns'] += 1
                        if outputs[0] != outputs[1]:
                            record['differences'] += 1
                            if len(record['examples']) < 10:
                                record['examples'].append({'round': turn['round'], 'id': identity,
                                                          'before': outputs[0], 'after': outputs[1]})
            finally:
                for group in workers:
                    for bot in group.values():
                        bot.stop()
            records.append(record)
            print(f"Game {record['game']}: {record['turns']} turns, {record['differences']} differences", flush=True)
    finally:
        for pool in pools:
            pool.close()
    summary = {'games': len(records), 'turns': sum(r['turns'] for r in records),
               'differences': sum(r['differences'] for r in records),
               'errors': sum(len(r['errors']) for r in records),
               'binary_sha256': [hashlib.sha256(b.read_bytes()).hexdigest() for b in binaries],
               'records': records}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(summary, indent=2) + '\n')
    print({k: v for k, v in summary.items() if k != 'records'}, flush=True)
    return int(bool(summary['differences'] or summary['errors']))


if __name__ == '__main__':
    raise SystemExit(main())

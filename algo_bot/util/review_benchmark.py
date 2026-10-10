#!/usr/bin/env python3
"""Audit saved benchmark warnings against the intentional small-helper trade gate.

Original records are preserved. This supports reviewing runs that omitted
--allow-favourable-trades; it never excuses body collisions or invalid actions.
"""
import argparse
import json
from pathlib import Path
from benchmark import visible_action_check


def review(folder: Path) -> dict:
    rows = [json.loads(line) for line in (folder/'matches.jsonl').read_text().splitlines()]
    accepted, failures = [], []
    for row in rows:
        for error in row['errors']:
            if error['team'] != row['candidate_team']:
                continue
            death = next((d for d in row['deaths'] if d['diagnostic'] == error.get('diagnostic')),None)
            if error['error'] == 'avoidable visible collision' and death and death['reason'] == 'H':
                trace = json.loads(Path(death['diagnostic']).read_text())
                check = visible_action_check(trace['stdin'],trace['stdout'])
                labels = dict(line.split(maxsplit=1) for line in trace['stdin'].splitlines()
                              if line.startswith(('ID ','LENGTH ','UNIT_COUNT ')))
                if (check.get('favourable_head_trade') and int(labels['ID']) > 1
                        and int(labels['LENGTH']) <= 3 and int(labels['UNIT_COUNT']) > 1):
                    accepted.append(error | {'verified_trade':check['favourable_head_trade']})
                    continue
            failures.append(error)
        if row['points'][row['candidate_team']]['max'] > 90000000:
            failures.append({'error':'candidate exceeds instruction margin','replay':row['replay']})
        for death in row['deaths']:
            if death['team'] == row['candidate_team'] and death['reason'] == 'A':
                failures.append({'error':'invalid candidate action','death':death})
    result = {'games':len(rows),'accepted_intentional_head_trades':accepted,'unresolved_failures':failures,
              'passed_trade_policy':not failures,'original_records_preserved':True}
    (folder/'head-trade-review.json').write_text(json.dumps(result,indent=2)+'\n')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder',type=Path)
    args = parser.parse_args()
    result = review(args.folder)
    print(json.dumps(result,indent=2))
    raise SystemExit(not result['passed_trade_policy'])

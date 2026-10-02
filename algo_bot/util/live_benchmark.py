#!/usr/bin/env python3
"""Plan, or explicitly queue, unranked tests for a verified active submission."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import subprocess


def opponents(leaderboard: list[dict], own_id: int, count: int) -> list[dict]:
    eligible = [team for team in leaderboard if team['id'] != own_id
                and team.get('hasBot') and not team.get('dev')]
    return sorted(eligible, key=lambda team: (-team['elo'], team['id']))[:count]


def verify_active(submissions: list[dict], expected: int) -> dict:
    active = [item for item in submissions if item['status'] == 'active']
    if len(active) != 1 or active[0]['id'] != expected:
        raise ValueError(f'expected active submission {expected}; found {[item["id"] for item in active]}')
    return active[0]


class API:
    def __init__(self):
        self.server = os.environ.get('UNSWBC_SERVER', 'https://game.battlecode.au').rstrip('/')
        self.key = os.environ.get('UNSWBC_KEY', '')
        if not self.server.startswith('https://') or not re.fullmatch(r'[A-Za-z0-9_-]+', self.key):
            raise ValueError('load .env first; UNSWBC_KEY and an HTTPS UNSWBC_SERVER are required')

    def request(self, path: str, body: dict | None = None):
        # Credentials stay off the process command line. No redirects are needed
        # for these JSON endpoints, and ambiguous POST failures are not retried.
        config = f'header = "Authorization: Bearer {self.key}"\n'
        command = ['curl', '--silent', '--show-error', '--fail-with-body',
                   '--proto', '=https', '--max-time', '30', '--config', '-',
                   self.server + '/api/v1/' + path]
        if body is not None:
            command += ['-H', 'Content-Type: application/json', '-d', json.dumps(body)]
        result = subprocess.run(command, input=config, text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError(f'{path}: {result.stderr.strip()} {result.stdout.strip()}')
        return json.loads(result.stdout)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--submission', type=int, required=True, help='expected active submission ID')
    parser.add_argument('--top', type=int, default=2, help='strongest eligible opponents, 1–5')
    parser.add_argument('--output', type=Path, required=True, help='fresh JSON evidence file')
    parser.add_argument('--queue', action='store_true', help='send the planned unranked challenges')
    args = parser.parse_args()
    if not 1 <= args.top <= 5 or args.submission <= 0:
        parser.error('top must be 1–5 and submission must be positive')
    if args.output.exists():
        parser.error('choose a fresh output file to preserve prior evidence')
    evidence = {}
    try:
        api = API()
        submission = verify_active(api.request('submissions'), args.submission)
        own_id = api.request('team')['team']['id']
        maps = api.request('maps')
        targets = opponents(api.request('leaderboard'), own_id, args.top)
        if not maps or len(targets) != args.top or len(maps) * len(targets) > 60:
            raise ValueError('insufficient maps/opponents or plan exceeds the 60-game hourly limit')
        evidence = {'submission': {key: submission[key] for key in ('id', 'name', 'version', 'sourceHash')},
                    'maps': [{'id': item['id'], 'name': item['name']} for item in maps],
                    'planned_games': len(maps) * len(targets), 'requests': [], 'queued': []}
        for team in targets:
            evidence['requests'].append({'opponent': {key: team[key] for key in ('id', 'name', 'elo')},
                'body': {'teamId': team['id'], 'ranked': False, 'mapIds': [item['id'] for item in maps]}})
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(evidence, indent=2) + '\n')
        if args.queue:
            for request in evidence['requests']:
                verify_active(api.request('submissions'), args.submission)
                response = api.request('battles', request['body'])
                evidence['queued'].append({'opponent': request['opponent'], 'response': response})
                args.output.write_text(json.dumps(evidence, indent=2) + '\n')
        print(f'{"Queued" if args.queue else "Planned"} {evidence["planned_games"]} unranked games; {args.output}')
        return 0
    except (ValueError, RuntimeError, OSError, KeyError) as error:
        if evidence:
            evidence['error'] = str(error)
            args.output.write_text(json.dumps(evidence, indent=2) + '\n')
        print(f'Live benchmark stopped: {error}')
        return 1


if __name__ == '__main__':
    raise SystemExit(main())

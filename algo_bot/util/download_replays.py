#!/usr/bin/env python3
"""Download every available game from the team's recent battle history."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('replays'))
    args = parser.parse_args()
    key = os.environ.get('UNSWBC_KEY')
    if not key:
        parser.error('Load .env first: set -a; source .env; set +a')
    server = os.environ.get('UNSWBC_SERVER', 'https://game.battlecode.au').rstrip('/')
    if not server.startswith('https://'):
        parser.error('UNSWBC_SERVER must use HTTPS')
    args.output.mkdir(parents=True, exist_ok=True)

    def fetch(path, destination):
        # curl drops Authorization when a replay redirects to another host.
        # Pass credentials through stdin rather than process arguments.
        temporary = destination.with_suffix(destination.suffix + '.part')
        try:
            result = subprocess.run(
                ['curl', '--fail', '--silent', '--show-error', '--location',
                 '--compressed', '--proto', '=https', '--proto-redir', '=https',
                 '--max-time', '60', '--retry', '3', '--retry-delay', '2',
                 '--header', '@-', '--output', str(temporary),
                 server + '/api/v1/' + path],
                input=f'Authorization: Bearer {key}\nOrigin: {server}\n',
                text=True, capture_output=True,
            )
            if result.returncode:
                raise RuntimeError(result.stderr.strip())
            if not temporary.stat().st_size:
                raise RuntimeError('Server returned an empty file')
            temporary.replace(destination)
        finally:
            temporary.unlink(missing_ok=True)
            time.sleep(0.6)

    def read_json(path, destination):
        fetch(path, destination)
        return json.loads(destination.read_text())

    battles = read_json('battles?limit=200', args.output / 'battles.json')
    if not isinstance(battles, list):
        raise RuntimeError('Unexpected battle history response')
    summary = {'battles': len(battles), 'downloaded': [], 'existing': [],
               'unavailable': [], 'errors': [], 'history_limit_reached': len(battles) >= 200}
    if summary['history_limit_reached']:
        print('WARNING: API returns at most 200 series; older history may be missing.', flush=True)
    seen = set()
    for battle in battles:
        battle_id = int(battle['id'])
        try:
            detail = read_json(f'battles/{battle_id}', args.output / f'battle-{battle_id}.json')
            games = detail.get('games') or [detail['match']]
            for game in games:
                game_id = int(game['id'])
                if game_id in seen:
                    continue
                seen.add(game_id)
                destination = args.output / f'{game_id}.replay'
                if destination.exists() and destination.stat().st_size:
                    summary['existing'].append(game_id)
                    continue
                if game.get('status') != 'completed' or game.get('hasReplay') is False:
                    summary['unavailable'].append(game_id)
                    continue
                try:
                    fetch(f'battles/{game_id}/replay', destination)
                    summary['downloaded'].append(game_id)
                    print(f'Downloaded game {game_id}', flush=True)
                except RuntimeError as error:
                    summary['errors'].append({'game': game_id, 'error': str(error)})
                    print(f'Could not download game {game_id}: {error}', flush=True)
        except (RuntimeError, ValueError, KeyError) as error:
            summary['errors'].append({'battle': battle_id, 'error': str(error)})
            print(f'Could not read battle {battle_id}: {error}', flush=True)
    (args.output / 'download-summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(f"{len(summary['downloaded'])} downloaded, {len(summary['existing'])} existing, "
          f"{len(summary['unavailable'])} unavailable, {len(summary['errors'])} errors", flush=True)
    return 1 if summary['errors'] or summary['history_limit_reached'] else 0


if __name__ == '__main__':
    raise SystemExit(main())

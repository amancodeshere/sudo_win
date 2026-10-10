import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('live_benchmark', Path(__file__).parents[1] / 'live_benchmark.py')
live = importlib.util.module_from_spec(spec)
spec.loader.exec_module(live)


class LiveBenchmarkTests(unittest.TestCase):
    def test_leaders_exclude_own_team_inactive_bots_and_developers(self):
        teams = [{'id': 925, 'elo': 2200, 'hasBot': True},
                 {'id': 2, 'elo': 2100, 'hasBot': False},
                 {'id': 3, 'elo': 2050, 'hasBot': True, 'dev': True},
                 {'id': 801, 'elo': 2022, 'hasBot': True},
                 {'id': 213, 'elo': 2006, 'hasBot': True}]
        self.assertEqual([team['id'] for team in live.opponents(teams[::-1], 925, 2)], [801, 213])

    def test_compilation_pending_wrong_or_multiple_active_versions_are_rejected(self):
        self.assertEqual(live.verify_active([{'id': 14399, 'status': 'active'}], 14399)['id'], 14399)
        for submissions in [[], [{'id': 14399, 'status': 'processing'}],
                            [{'id': 14151, 'status': 'active'}],
                            [{'id': 14399, 'status': 'active'}, {'id': 14151, 'status': 'active'}]]:
            with self.assertRaises(ValueError):
                live.verify_active(submissions, 14399)

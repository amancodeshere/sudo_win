import importlib.util
from pathlib import Path
import unittest
import tempfile

spec = importlib.util.spec_from_file_location('audit', Path(__file__).parents[1] / 'analyze_replays.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class ReplayBoardTests(unittest.TestCase):
    def board(self, extra=''):
        edges = '\n'.join(f'EDGE {r*5+x} 0 -1' for r in range(8) for x in range(4))
        return audit.Board('MAP 4 4\nMAP_NAME test\n' + edges + '\n' + extra)

    def test_respawn_gaps_do_not_mean_initial_pearls(self):
        board = self.board('TILE 0 0 10 20\nTILE 1 0 0 0\n')
        self.assertFalse(board.pearls)
        self.assertIn((0, 0), board.countdown)
        self.assertNotIn((1, 0), board.countdown)

    def test_padding_edges_do_not_overwrite_wrapped_boundaries(self):
        board = self.board('EDGE 4 1 -1\nEDGE 40 1 -1\nEDGE 9 1 -1\n')
        self.assertEqual(board.step((0, 0), 'N'), (0, 3))
        self.assertEqual(board.step((0, 0), 'W'), (3, 0))

    def test_portals_preserve_heading(self):
        board = self.board('EDGE 6 2 3\nEDGE 18 2 3\n')
        self.assertEqual(board.step((0, 0), 'E'), (3, 1))
        self.assertEqual(board.step((3, 1), 'W'), (0, 0))

    def test_remote_portal_exit_requires_pair_discovery_and_current_visibility(self):
        board = self.board('EDGE 6 2 3\nEDGE 18 2 3\n')
        # On this small board the exit is visible, but the partner can still
        # be absent from the supplied observation history.
        option = board.portal_options((0, 0), set())[0]
        self.assertFalse(option['partner_known'])
        self.assertTrue(option['exit_visible'])
        option = board.portal_options((0, 0), {('V', 3, 1)})[0]
        self.assertTrue(option['partner_known'])
        board.dragons[0] = {'team': 'B', 'body': [(3, 1)], 'facing': 'W'}
        self.assertEqual(board.portal_options((0, 0), set())[0]['occupied'], 0)

    def test_submission_filter_uses_our_colour_and_skips_before_export(self):
        replay = audit.SCHEMA.Replay.new_message(formatVersion=2, botA='111', botB='222')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / '1.replay'
            path.write_bytes(replay.to_bytes_packed())
            output = Path(directory) / 'not-created'
            self.assertIsNone(audit.analyze(path, {'ours': 'B'}, output, submission=111))
            self.assertIsNone(audit.analyze(path, {'ours': 'A'}, output, submission=222))
            self.assertFalse(output.exists())

    def test_head_collision_records_executed_step_and_attacker(self):
        board = self.board('DRAGON 0 3 1 1 1 2 1 3\nDRAGON 1 3 3 1 3 2 3 3\n')
        edges = '\n'.join(f'EDGE {r*5+x} 0 -1' for r in range(8) for x in range(4))
        replay = audit.SCHEMA.Replay.new_message(formatVersion=2, botA='14465')
        replay.map = 'MAP 4 4\n' + edges + '\nDRAGON 0 3 1 1 1 2 1 3\nDRAGON 1 3 3 1 3 2 3 3\n'
        self.assertEqual(len(board.dragons), 2)
        events = replay.init('events', 6)
        events[0].init('roundStart').round = 0
        events[1].init('turnStart').id = 1
        action = events[2].init('dragonAction')
        action.id = 1
        action.action.move = ['west', 'west']
        update = events[3].init('dragonUpdate')
        update.id, update.facing = 1, 'west'
        update.head.x, update.head.y = 2, 1
        update.tail.x, update.tail.y = 3, 2
        for index, identity in enumerate((0, 1), start=4):
            death = events[index].init('dragonDeath')
            death.id, death.reason = identity, 'hitHeadToHead'
        replay.result.init('teamA')
        replay.result.init('teamB')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / '1.replay'
            path.write_bytes(replay.to_bytes_packed())
            record = audit.analyze(path, {'ours': 'A'}, Path(directory), submission=14465)
            self.assertEqual(record['deaths'][0]['collision_step'], 2)
            self.assertEqual(record['deaths'][0]['attacking_turn']['team'], 'B')
            self.assertEqual(record['deaths'][0]['attacking_turn']['length'], 3)

if __name__ == '__main__':
    unittest.main()

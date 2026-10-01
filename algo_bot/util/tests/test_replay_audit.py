import importlib.util
from pathlib import Path
import unittest

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

if __name__ == '__main__':
    unittest.main()

import importlib.util
import pathlib
import tempfile
import unittest

SCRIPT = pathlib.Path(__file__).parents[1] / "benchmark.py"
spec = importlib.util.spec_from_file_location("benchmark", SCRIPT)
benchmark = importlib.util.module_from_spec(spec)
spec.loader.exec_module(benchmark)


class BenchmarkTests(unittest.TestCase):
    @staticmethod
    def observation(length=3):
        lines = ["ID 0", "TEAM A", "MAP 10 10", "UNIT_LIMIT 64", "ROUND 1", "DIR N",
                 f"LENGTH {length}", "UNIT_COUNT 1", "NUM_MSGS 0", "ECHOES 0 0 0 0 0"]
        lines += [f"{x} {y} 0 -1" for y in range(2, 9) for x in range(2, 9)]
        parts = ["A 0 5 5 N 1", "A 0 5 6 N 0", "A 0 5 7 N 0"][:length]
        lines += [f"DRAGON_BODIES {len(parts)}", *parts]
        lines += [". . . . . . ."] * 8 + [". . . . . . . ."] * 7
        return "\n".join(lines) + "\n"

    def test_visible_collision_gate_checks_own_body_and_sprint_intermediate_steps(self):
        observation = self.observation()
        safe = benchmark.visible_action_check(observation, "MOVE N\n")
        self.assertFalse(safe["avoidable_collision"])
        blocked = benchmark.visible_action_check(observation, "MOVE S\n")
        self.assertTrue(blocked["avoidable_collision"])
        self.assertEqual(blocked["fatal_step"], 1)
        reverse = benchmark.visible_action_check(observation, "MOVE NS\n")
        self.assertTrue(reverse["avoidable_collision"])
        self.assertEqual(reverse["fatal_step"], 2)
        short = benchmark.visible_action_check(self.observation(2), "MOVE NN\n")
        self.assertEqual(short["fatal_reason"], "unaffordable sprint")
    def test_head_trade_classifier_keeps_unfunded_and_unfavourable_collisions_fatal(self):
        observation = self.observation(2).replace('UNIT_COUNT 1', 'UNIT_COUNT 2')
        observation = observation.replace('6 5 0 -1', '6 5 1 -1')
        enemies = '\n'.join(['B 9 7 5 N 1', 'B 9 7 6 N 0', 'B 9 7 7 N 0', 'B 9 7 8 N 0'])
        observation = observation.replace('DRAGON_BODIES 2', 'DRAGON_BODIES 6')
        observation = observation.replace('A 0 5 6 N 0', 'A 0 5 6 N 0\n' + enemies)
        result = benchmark.visible_action_check(observation, 'MOVE EE\n')
        self.assertTrue(result['avoidable_collision'])  # Still gated without explicit trial opt-in.
        self.assertEqual(result['favourable_head_trade']['enemy_visible_length'], 4)
        for invalid in (observation.replace('6 5 1 -1', '6 5 0 -1'),
                        observation.replace('UNIT_COUNT 2', 'UNIT_COUNT 1'),
                        observation.replace('B 9', 'A 9'),
                        observation.replace('B 9 7 8 N 0', 'B 8 7 8 N 0')):
            self.assertNotIn('favourable_head_trade', benchmark.visible_action_check(invalid, 'MOVE EE\n'))

    def test_submission_excludes_test_sources_and_hashes_headers(self):
        with tempfile.TemporaryDirectory() as work:
            root = pathlib.Path(work)
            bot = root / "bot"
            for directory in ("src", "include", "tests"):
                (bot / directory).mkdir(parents=True)
            (bot / "bot.toml").write_text('[project]\nlanguage="c++"\ninclude=["src/*.cpp","include/*.h"]\n')
            (bot / "src/main.cpp").write_text("int main() {}")
            header = bot / "include/config.h"
            header.write_text("constexpr int depth=3;")
            (bot / "tests/test.cpp").write_text("int main() {}")
            staged, first = benchmark.stage(bot, root / "staged")
            self.assertTrue((staged / "src/main.cpp").exists())
            self.assertFalse((staged / "tests").exists())
            header.write_text("constexpr int depth=4;")
            changed, second = benchmark.stage(bot, root / "staged")
            self.assertNotEqual(first, second)
            self.assertNotEqual(staged, changed)

    def test_small_sample_percentiles_keep_worst_turn(self):
        self.assertEqual(benchmark.percentile([4, 1, 9], 95), 9)
        self.assertEqual(benchmark.percentile([4, 1, 9], 50), 4)
        self.assertEqual(benchmark.percentile([], 95), 0)

    def test_action_metrics_require_one_well_formed_action(self):
        self.assertEqual(benchmark.action_metrics(b"MOVE NES\nPROTOCOL 3\n")[0],
                         {"sprint": 1, "movement_steps": 3})
        self.assertEqual(benchmark.action_metrics(b"SPLIT 3\n")[0], {"split": 1, "split_segments": 3})
        for bad in (b"", b"MOVE N\nMOVE E\n", b"MOVE banana\n", b"SPLIT -2\n"):
            self.assertIsNotNone(benchmark.action_metrics(bad)[1])


if __name__ == "__main__":
    unittest.main()

import importlib.util
import pathlib
import tempfile
import unittest

SCRIPT = pathlib.Path(__file__).parents[1] / "benchmark.py"
spec = importlib.util.spec_from_file_location("benchmark", SCRIPT)
benchmark = importlib.util.module_from_spec(spec)
spec.loader.exec_module(benchmark)


class BenchmarkTests(unittest.TestCase):
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

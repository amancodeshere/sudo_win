import importlib.util
import pathlib
import tempfile
import unittest
import zipfile

SCRIPT = pathlib.Path(__file__).parents[1] / "prepare_submission.py"
spec = importlib.util.spec_from_file_location("prepare_submission", SCRIPT)
submission = importlib.util.module_from_spec(spec)
spec.loader.exec_module(submission)
BOT = pathlib.Path(__file__).parents[2]


class SubmissionTests(unittest.TestCase):
    def test_profiles_are_isolated_and_archive_contains_only_submission_sources(self):
        original = (BOT / "include/sudo_win/config/config.h").read_bytes()
        with tempfile.TemporaryDirectory() as work:
            root = pathlib.Path(work)
            stable = submission.prepare(BOT, root / "stable", "stable")
            experimental = submission.prepare(BOT, root / "experimental", "experimental")
            growth = submission.prepare(BOT, root / "growth", "growth")
            combat = submission.prepare(BOT, root / "combat", "combat")
            self.assertEqual(stable["effective_flags"]["enable_favourable_trades"], "false")
            self.assertEqual(combat["effective_flags"]["enable_favourable_trades"], "true")
            self.assertEqual(combat["effective_flags"]["enable_funded_sprint_priority"], "false")
            self.assertEqual(stable["effective_flags"]["enable_splitting"], "false")
            self.assertEqual(experimental["effective_flags"]["enable_splitting"], "true")
            self.assertEqual(stable["effective_flags"]["enable_growth_splitting"], "true")
            self.assertEqual(stable["effective_flags"]["enable_sonar"], "true")
            self.assertEqual(stable["effective_flags"]["enable_helper_portals"], "true")
            self.assertEqual(stable["effective_flags"]["enable_queen_hunting"], "true")
            for profile, flag in (("no-growth", "enable_growth_splitting"),
                                  ("no-sonar", "enable_sonar"),
                                  ("no-helper-portals", "enable_helper_portals"),
                                  ("no-hunting", "enable_queen_hunting"),
                                  ("no-corridors", "enable_queen_corridors"),
                                  ("no-territory", "enable_territorial_growth"),
                                  ("no-farms", "enable_champion_farms"),
                                  ("no-portal-routes", "enable_portal_routing"),
                                  ("no-economics", "enable_movement_economics"),
                                  ("no-sprint", "enable_sprinting")):
                ablation = submission.prepare(BOT, root / profile, profile)
                differences = [key for key in stable["effective_flags"]
                               if stable["effective_flags"][key] != ablation["effective_flags"][key]]
                self.assertEqual(differences, [flag])
            self.assertEqual(stable["effective_flags"]["enable_long_sprint_threats"], "false")
            self.assertEqual(stable["effective_flags"]["enable_pocket_priority"], "false")
            self.assertEqual(experimental["effective_flags"]["enable_pocket_priority"], "true")
            self.assertEqual(experimental["effective_flags"]["enable_growth_splitting"], "true")
            self.assertEqual(growth["effective_flags"]["enable_splitting"], "false")
            self.assertEqual(growth["effective_flags"]["enable_growth_splitting"], "true")
            with zipfile.ZipFile(stable["archive"]) as archive:
                self.assertIn("src/main.cpp", archive.namelist())
                self.assertIn("src/planner/simulation.cpp", archive.namelist())
                self.assertFalse(any(name.startswith(("tests/", "lib/", "util/")) for name in archive.namelist()))
                self.assertEqual(archive.testzip(), None)
            repeated = submission.prepare(BOT, root / "stable-again", "stable")
            self.assertEqual(stable["archive_sha256"], repeated["archive_sha256"])
            with self.assertRaises(ValueError):
                submission.prepare(BOT, root / "stable", "stable")
        self.assertEqual((BOT / "include/sudo_win/config/config.h").read_bytes(), original)


if __name__ == "__main__":
    unittest.main()

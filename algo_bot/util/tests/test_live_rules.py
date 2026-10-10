import unittest
import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location('verify_rules', Path(__file__).parents[1] / 'verify_rules.py')
rules_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rules_module)


class LiveRulesTests(unittest.TestCase):
    def test_official_judge_matches_live_rules_in_both_colours(self):
        rules = rules_module.verify_engine_rules()
        self.assertTrue(rules['queen_scoring'])
        self.assertTrue(rules['free_sprint_steps'])
        self.assertTrue(rules['fixed_start_length'])
        self.assertTrue(rules['dead_queen_zero'])
        self.assertTrue(rules['cyclic_turn_order'])
        self.assertTrue(rules['controlled_donation_food'])

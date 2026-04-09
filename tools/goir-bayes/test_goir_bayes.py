import unittest
import os
import json
from goir_bayes import BetaPosterior, BeliefStore

class TestGoirBayes(unittest.TestCase):
    def test_beta_posterior_mean(self):
        bp = BetaPosterior(alpha=2, beta=2)
        self.assertEqual(bp.mean(), 0.5)

        bp = BetaPosterior(alpha=1, beta=9)
        self.assertEqual(bp.mean(), 0.1)

    def test_beta_posterior_update(self):
        bp = BetaPosterior(alpha=1, beta=1)
        bp.update(passed=10, failed=0)
        self.assertEqual(bp.alpha, 11)
        self.assertEqual(bp.beta, 1)
        self.assertGreater(bp.mean(), 0.5)

    def test_weighted_update(self):
        bp1 = BetaPosterior(alpha=1, beta=1)
        bp1.update(passed=10, failed=0, w=1.0)

        bp2 = BetaPosterior(alpha=1, beta=1)
        bp2.update(passed=10, failed=0, w=0.5)

        self.assertGreater(bp1.mean(), bp2.mean())

    def test_belief_store_save_load(self):
        store_file = "test_beliefs.json"
        if os.path.exists(store_file):
            os.remove(store_file)

        store = BeliefStore(store_file)
        bp = store.get_belief("task1")
        bp.update(5, 0)
        store.save()

        store2 = BeliefStore(store_file)
        bp2 = store2.get_belief("task1")
        self.assertEqual(bp2.alpha, 6)
        self.assertEqual(bp2.beta, 1)

        if os.path.exists(store_file):
            os.remove(store_file)

    def test_thompson_sampling(self):
        # We can't easily test randomness, but we can test that it returns one of the actions
        bp = BetaPosterior(alpha=1, beta=1)
        sample = bp.sample()
        self.assertGreaterEqual(sample, 0.0)
        self.assertLessEqual(sample, 1.0)

if __name__ == "__main__":
    unittest.main()

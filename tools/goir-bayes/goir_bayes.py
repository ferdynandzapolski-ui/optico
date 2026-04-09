#!/usr/bin/env python3
import json
import argparse
import sys
import random
import os

class BetaPosterior:
    def __init__(self, alpha=1.0, beta=1.0):
        self.alpha = alpha
        self.beta = beta

    def mean(self) -> float:
        return self.alpha / (self.alpha + self.beta)

    def update(self, passed: int, failed: int, w: float = 1.0):
        self.alpha += w * passed
        self.beta += w * failed

    def sample(self) -> float:
        return random.betavariate(self.alpha, self.beta)

    def to_dict(self):
        return {"alpha": self.alpha, "beta": self.beta}

    @staticmethod
    def from_dict(d):
        return BetaPosterior(d.get("alpha", 1.0), d.get("beta", 1.0))

class BeliefStore:
    def __init__(self, filepath):
        self.filepath = filepath
        self.beliefs = {}
        if os.path.exists(filepath):
            try:
                with open(filepath, 'r') as f:
                    data = json.load(f)
                    for task_id, belief_data in data.items():
                        self.beliefs[task_id] = BetaPosterior.from_dict(belief_data)
            except Exception as e:
                print(f"Error loading belief store: {e}", file=sys.stderr)

    def save(self):
        data = {task_id: belief.to_dict() for task_id, belief in self.beliefs.items()}
        with open(self.filepath, 'w') as f:
            json.dump(data, f, indent=2)

    def get_belief(self, task_id, default_alpha=1.0, default_beta=1.0):
        if task_id not in self.beliefs:
            self.beliefs[task_id] = BetaPosterior(default_alpha, default_beta)
        return self.beliefs[task_id]

def cmd_init(args):
    store = BeliefStore(args.store)
    store.beliefs[args.task] = BetaPosterior(args.alpha, args.beta)
    store.save()
    print(f"Initialized belief for task '{args.task}': alpha={args.alpha}, beta={args.beta}")

def cmd_update(args):
    store = BeliefStore(args.store)
    belief = store.get_belief(args.task)

    with open(args.evidence, 'r') as f:
        evidence = json.load(f)

    # Example evidence structure: {"lit": {"passed": 10, "failed": 0}, "microc": {"passed": 5, "failed": 1}}
    # We apply weights: lit=1.0, microc=0.7 (as per report suggestion)
    weights = {"lit": 1.0, "microc": 0.7}

    total_passed = 0
    total_failed = 0

    for source, results in evidence.items():
        if isinstance(results, dict):
            passed = results.get("passed", 0)
            failed = results.get("failed", 0)
            w = weights.get(source, 1.0)
            belief.update(passed, failed, w)
            total_passed += passed
            total_failed += failed

    store.save()
    print(f"Updated belief for task '{args.task}' with evidence from {args.evidence}.")
    print(f"New mean: {belief.mean():.4f}")

def cmd_summarize(args):
    store = BeliefStore(args.store)
    if args.task not in store.beliefs:
        print(f"No belief found for task '{args.task}'")
        return

    belief = store.beliefs[args.task]
    mean = belief.mean()
    print(f"--- Belief Summary: {args.task} ---")
    print(f"Parameters: alpha={belief.alpha:.2f}, beta={belief.beta:.2f}")
    print(f"Posterior Mean (Confidence): {mean:.4f}")

    if mean >= args.threshold:
        print(f"RECOMMENDATION: MERGE (Confidence {mean:.4f} >= {args.threshold})")
    else:
        print(f"RECOMMENDATION: DO NOT MERGE (Confidence {mean:.4f} < {args.threshold})")

def cmd_select_action(args):
    store = BeliefStore(args.store)
    # actions is a comma-separated list of task_ids or action_ids
    actions = args.actions.split(',')

    samples = {}
    for action in actions:
        belief = store.get_belief(action)
        samples[action] = belief.sample()

    best_action = max(samples, key=samples.get)
    print(f"Selected action: {best_action} (Sampled value: {samples[best_action]:.4f})")

def main():
    parser = argparse.ArgumentParser(description="GOIR Bayesian Reasoning Tool")
    parser.add_argument("--store", default="beliefs.json", help="Path to JSON belief store")
    subparsers = parser.add_subparsers(dest="command")

    init_parser = subparsers.add_parser("init", help="Initialize a task belief")
    init_parser.add_argument("--task", required=True)
    init_parser.add_argument("--alpha", type=float, default=1.0)
    init_parser.add_argument("--beta", type=float, default=1.0)

    update_parser = subparsers.add_parser("update", help="Update belief with evidence")
    update_parser.add_argument("--task", required=True)
    update_parser.add_argument("--evidence", required=True, help="Path to JSON evidence packet")

    summarize_parser = subparsers.add_parser("summarize", help="Show belief summary")
    summarize_parser.add_argument("--task", required=True)
    summarize_parser.add_argument("--threshold", type=float, default=0.85)

    select_parser = subparsers.add_parser("select-action", help="Thompson sampling for action selection")
    select_parser.add_argument("--actions", required=True, help="Comma-separated list of actions/tasks")

    args = parser.parse_args()

    if args.command == "init":
        cmd_init(args)
    elif args.command == "update":
        cmd_update(args)
    elif args.command == "summarize":
        cmd_summarize(args)
    elif args.command == "select-action":
        cmd_select_action(args)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()

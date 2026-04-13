import argparse
import json
import math
import random
from dataclasses import dataclass

@dataclass
class Beta:
    a: float
    b: float
    def mean(self) -> float:
        return self.a / (self.a + self.b)

def beta_update(post: Beta, passed: int, failed: int, w: float = 1.0) -> Beta:
    return Beta(post.a + w * passed, post.b + w * failed)

def patch_log_likelihood(patch_results, eps=1e-6):
    ll = 0.0
    for w, ok in patch_results:
        ll += w * math.log((1 - eps) if ok else eps)
    return ll

def bma_select(patches, priors):
    scores = {}
    for pid, results in patches.items():
        scores[pid] = patch_log_likelihood(results) + priors.get(pid, 0.0)
    m = max(scores.values())
    Z = sum(math.exp(v-m) for v in scores.values())
    post = {pid: math.exp(v-m)/Z for pid, v in scores.items()}
    best = max(post, key=post.get)
    return best, post

def main():
    parser = argparse.ArgumentParser(description="GOIR Bayesian Belief Toolkit")
    subparsers = parser.add_subparsers(dest="command")

    init_parser = subparsers.add_parser("init")
    init_parser.add_argument("--out", default="beliefs.json")

    update_parser = subparsers.add_parser("update")
    update_parser.add_argument("--beliefs", default="beliefs.json")
    update_parser.add_argument("--evidence", required=True)

    bma_parser = subparsers.add_parser("bma")
    bma_parser.add_argument("--patches", required=True)

    args = parser.parse_parser_args() if hasattr(parser, "parse_parser_args") else parser.parse_args()

    if args.command == "init":
        # Risk-aware priors
        beliefs = {
            "inv.check_coverage": {"alpha": 4.0, "beta": 3.0}, # Medium
            "inv.shadow_sync": {"alpha": 4.0, "beta": 3.0},   # Medium
            "inv.memcpy_typed_ok": {"alpha": 2.0, "beta": 5.0}, # High
            "inv.ptrint_policy_ok": {"alpha": 2.0, "beta": 5.0}, # High
            "inv.cheri_elision_safe": {"alpha": 1.0, "beta": 6.0} # Very High
        }
        with open(args.out, "w") as f:
            json.dump(beliefs, f, indent=2)
        print(f"Initialized beliefs in {args.out}")

    elif args.command == "update":
        with open(args.beliefs, "r") as f:
            beliefs = json.load(f)
        with open(args.evidence, "r") as f:
            evidence = json.load(f)

        for inv, res in evidence.items():
            if inv in beliefs:
                b = Beta(beliefs[inv]["alpha"], beliefs[inv]["beta"])
                new_b = beta_update(b, res.get("pass", 0), res.get("fail", 0), res.get("w", 1.0))
                beliefs[inv] = {"alpha": new_b.a, "beta": new_b.b}

        with open(args.beliefs, "w") as f:
            json.dump(beliefs, f, indent=2)
        print(f"Updated beliefs in {args.beliefs}")

    elif args.command == "bma":
        with open(args.patches, "r") as f:
            data = json.load(f)
        best, posteriors = bma_select(data["patches"], data.get("priors", {}))
        print(json.dumps({"best": best, "posteriors": posteriors}, indent=2))

if __name__ == "__main__":
    main()

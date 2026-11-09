#!/usr/bin/env python3
"""Finds imported signatures that make PC callers decompile worse than Ghidra's own analysis did.
Compares two ExportDecomp outputs (baseline = before type transfer, current), counts register/stack
artifacts per function, and ranks matched callees by the share of their callers that got worse.
Appends suspects to knowledge/raw/transfer/reset_signatures.txt (used by ResetSignatures.java).
Usage: suspect_signatures.py <baseline-src> <current-src> [min_share=0.4] [min_callers=2]"""
import collections, os, re, sqlite3, sys
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ART = re.compile(r"\b(in_stack|unaff_|in_E[A-Z]X)\w*")

def funcs(d):
    r = {}
    for f in os.listdir(d):
        t = open(os.path.join(d, f), errors="replace").read()
        for b in re.split(r"\n(?=// \S+ @ [0-9a-f]+\n)", t):
            m = re.match(r"// (\S+) @ ([0-9a-f]+)", b)
            if m: r[int(m.group(2), 16)] = len(ART.findall(b))
    return r

def main():
    base, cur = funcs(sys.argv[1]), funcs(sys.argv[2])
    share = float(sys.argv[3]) if len(sys.argv) > 3 else 0.4
    mincall = int(sys.argv[4]) if len(sys.argv) > 4 else 2
    db = sqlite3.connect(os.path.join(ROOT, "knowledge", "knowledge.db"))
    callees = collections.defaultdict(set)
    for x, y in db.execute("SELECT caller, callee FROM pc_calls"): callees[x].add(y)
    matched = {p for p, in db.execute("SELECT pc_addr FROM matches WHERE confidence >= 0.6")}
    path = os.path.join(ROOT, "knowledge", "raw", "transfer", "reset_signatures.txt")
    done = {int(l, 16) for l in open(path).read().split()} if os.path.exists(path) else set()
    worse = {k for k in cur if k in base and cur[k] > base[k]}
    bad, seen = collections.Counter(), collections.Counter()
    for k in cur:
        for c in callees[k]:
            if c in matched and c not in done:
                seen[c] += 1
                if k in worse: bad[c] += 1
    new = sorted(c for c in bad if seen[c] >= mincall and bad[c] >= mincall and bad[c] / seen[c] >= share)
    with open(path, "a") as f:
        f.writelines(f"{c:X}\n" for c in new)
    print(f"worse functions {len(worse)}; {len(new)} new suspect signatures appended to {path}")

if __name__ == "__main__":
    main()

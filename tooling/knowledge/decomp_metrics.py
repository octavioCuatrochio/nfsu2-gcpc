#!/usr/bin/env python3
"""Readability metrics for an ExportDecomp.java output folder (lower is better unless marked).

Usage: decomp_metrics.py <src-dir> [<src-dir> ...]   (several dirs are shown side by side)
"""
import os, re, sys

METRICS = [
    ("functions", None, "number of functions"),
    ("named functions %", None, "functions with a real name (higher is better)"),
    ("calls to FUN_/unnamed", re.compile(r"\bFUN_[0-9a-f]{8}\s*\("), "per 1000 lines"),
    ("raw casts *(T *)(x + n)", re.compile(r"\*\([\w ]+\*+\)\s*\([^()]*\+\s*(0x[0-9a-f]+|\d+)\)"), "per 1000 lines"),
    ("'undefined' types", re.compile(r"\bundefined\d?\b"), "per 1000 lines"),
    ("register artifacts in_/unaff_", re.compile(r"\b(in_|unaff_)\w+"), "per 1000 lines"),
    ("this[N] overflows", re.compile(r"\bthis\[\d+\]"), "per 1000 lines"),
    ("generic fields ->field_0x", re.compile(r"(->|\.)field_0x[0-9a-f]+"), "per 1000 lines"),
    ("named fields ->name", re.compile(r"(->|\.)(?!field_0x|_vptr)[A-Za-z_]\w*\b(?!\s*\()"), "per 1000 lines (higher is better)"),
    ("param_N parameters", re.compile(r"\bparam_\d+\b"), "per 1000 lines"),
]


def measure(src):
    text, nfun, named = [], 0, 0
    for root, _, files in os.walk(src):
        for fn in files:
            if not fn.endswith(".c"): continue
            with open(os.path.join(root, fn), errors="replace") as f:
                t = f.read()
            text.append(t)
            for name in re.findall(r"^// (\S+) @ [0-9a-f]+$", t, re.M):
                nfun += 1
                base = name.rsplit("::", 1)[-1]
                if not re.match(r"(FUN|thunk_FUN|LAB|SUB)_[0-9a-f]+$", base): named += 1
    t = "\n".join(text)
    lines = t.count("\n") or 1
    res = {"functions": nfun, "named functions %": 100.0 * named / max(1, nfun)}
    for k, rx, _ in METRICS:
        if rx is not None:
            res[k] = 1000.0 * len(rx.findall(t)) / lines
    return res


def main():
    dirs = sys.argv[1:]
    rs = [measure(d) for d in dirs]
    w = max(len(k) for k, _, _ in METRICS)
    print(" " * (w + 2) + "".join(f"{os.path.basename(os.path.dirname(d.rstrip('/'))) or d:>16}" for d in dirs))
    for k, _, unit in METRICS:
        print(f"{k:<{w}}  " + "".join(f"{r[k]:>16.1f}" if isinstance(r[k], float) else f"{r[k]:>16}" for r in rs) + f"   {unit}")


if __name__ == "__main__":
    main()

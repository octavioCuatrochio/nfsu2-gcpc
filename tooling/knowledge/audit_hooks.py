#!/usr/bin/env python3
"""Finds hooked PC functions whose callers rely on registers the x86 ABI does not preserve.

MSVC (with whole-program optimization) sometimes keeps using ECX/EDX after calling a function it knows does not
touch them. A C++ replacement does touch them -> the caller reads garbage (seen: GetDataOffset 5046F0, caller
reads [ecx+0x2c] right after the call). For each address (default: every hook in replacements.cpp) this scans
every direct call site and reports callers that READ ecx/edx after the call before WRITING them.
Such hooks must be registered with a register-preserving thunk (see hooks.h: Replacement::preserve).

Usage: audit_hooks.py [hex addresses...]
"""
import os, re, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXE = os.path.join(ROOT, "pc", "SPEED2.EXE")
REPL = os.path.join(ROOT, "port", "host", "win32-hook", "replacements.cpp")
REGS = {"ecx": ("ecx", "cx", "cl", "ch"), "edx": ("edx", "dx", "dl", "dh")}


def hooked_addresses():
    return [int(a, 16) for a in re.findall(r"\{0x([0-9A-Fa-f]{6}), \(const void \*\)", open(REPL).read())]


TEXT_VA, TEXT_OFF, TEXT_SIZE = 0x401000, 0x1000, 0x381AF1


def call_index(data):
    """One pass over .text: target VA -> every E8 byte whose rel32 points there (aligned or not)."""
    text = data[TEXT_OFF:TEXT_OFF + TEXT_SIZE]
    index, i, end = {}, text.find(b"\xe8"), len(text) - 5
    unpack = struct.unpack_from
    while 0 <= i <= end:
        t = (TEXT_VA + i + 5 + unpack("<i", text, i + 1)[0]) & 0xFFFFFFFF
        index.setdefault(t, []).append(TEXT_VA + i)
        i = text.find(b"\xe8", i + 1)
    return index


class Sweep:
    """Linear objdump sweep of .text, run once. Sites the sweep decodes at a different alignment fall back to a
    per-site objdump (same result as before, just rarer)."""
    def __init__(self):
        out = subprocess.run(["objdump", "-d", "--no-show-raw-insn", "-M", "intel", f"--start-address={hex(TEXT_VA)}",
                              f"--stop-address={hex(TEXT_VA + TEXT_SIZE)}", EXE], capture_output=True, text=True).stdout
        self.addrs, self.insns = [], []
        for l in out.splitlines():
            m = re.match(r"\s+([0-9a-f]+):\s*(.*)", l)
            if m:
                self.addrs.append(int(m.group(1), 16))
                self.insns.append(m.group(2).strip())
        self.pos = {a: k for k, a in enumerate(self.addrs)}

    def at(self, va, n=0x30):
        k = self.pos.get(va)
        if k is None:
            return disasm(va, n)
        out = []
        while k < len(self.addrs) and self.addrs[k] < va + n:
            out.append(self.insns[k])
            k += 1
        return out


def disasm(va, n=0x30):
    out = subprocess.run(["objdump", "-d", "--no-show-raw-insn", "-M", "intel", f"--start-address={hex(va)}",
                          f"--stop-address={hex(va + n)}", EXE], capture_output=True, text=True).stdout
    return [l.split(":", 1)[1].strip() for l in out.splitlines() if re.match(r"\s+[0-9a-f]+:", l)]


def uses_before_def(insns, reg):
    names = REGS[reg]
    for ins in insns[1:]:                      # skip the call itself
        if ins.startswith(".") or "(bad)" in ins: return False   # misaligned / data: stop
        m = re.match(r"(\w+)\s*(.*)", ins)
        if not m: continue
        op, args = m.group(1), m.group(2)
        if op in ("ret", "jmp") or op.startswith("j") or op == "call":
            return op == "call" and reg == "ecx" and False   # stop at control flow
        parts = [p.strip() for p in args.split(",")]
        dst = parts[0] if parts else ""
        srcs = " ".join(parts[1:]) if len(parts) > 1 else ""
        reads = any(re.search(rf"\b{r}\b", srcs) for r in names) or \
            any(re.search(rf"\[[^\]]*\b{r}\b", dst) for r in names) or \
            (op not in ("mov", "lea", "movzx", "movsx", "pop") and any(re.search(rf"\b{r}\b", dst) for r in names))
        if op.startswith("."): return False         # misaligned / data: stop
        if op == "xor" and len(parts) == 2 and parts[0] == parts[1]: reads = False
        if op == "or" and len(parts) == 2 and parts[1] in ("0xffffffff", "-1"): reads = False   # pure write
        if op == "and" and len(parts) == 2 and parts[1] == "0x0": reads = False
        if reads: return True
        if any(re.fullmatch(r, dst) for r in names) and op in ("mov", "lea", "movzx", "movsx", "pop", "xor", "or", "and"):
            return False                       # overwritten first
    return False


def main():
    data = open(EXE, "rb").read()
    targets = [int(a, 16) for a in sys.argv[1:]] or hooked_addresses()
    index, sweep = call_index(data), Sweep()
    bad = 0
    for t in targets:
        hits = []
        for site in index.get(t, []):
            ins = sweep.at(site)
            for reg in REGS:
                if uses_before_def(ins, reg):
                    hits.append(f"{site:06X}:{reg}")
        if hits:
            bad += 1
            print(f"{t:06X}: callers rely on preserved registers -> {', '.join(hits[:8])}" + (" ..." if len(hits) > 8 else ""))
    print(f"{len(targets)} functions audited, {bad} need a register-preserving thunk")


if __name__ == "__main__":
    main()

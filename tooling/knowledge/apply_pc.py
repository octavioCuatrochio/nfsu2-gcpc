#!/usr/bin/env python3
"""Writes knowledge/raw/pc/apply.tsv for ApplyKnowledge.java: what knowledge.db knows about PC addresses.
Only matches with confidence >= MIN_CONF are applied as names; everything goes into the comment."""
import os, sqlite3

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DB = os.path.join(ROOT, "knowledge", "knowledge.db")
OUT = os.path.join(ROOT, "knowledge", "raw", "pc", "apply.tsv")
MIN_CONF = 0.6


def clean(s):
    return (s or "").replace("\t", " ").replace("\n", "\\n")


def main():
    db = sqlite3.connect(DB)
    rows = []
    for pc, gc, name, sig, src, method, conf, re_name, re_notes in db.execute("""
        SELECT m.pc_addr, m.gc_addr, g.full_name, g.signature, g.src_file, m.method, m.confidence, r.name, r.notes
        FROM matches m JOIN gc_functions g ON g.addr = m.gc_addr LEFT JOIN re_functions r ON r.addr = m.pc_addr
        WHERE m.confidence >= ?""", (MIN_CONF,)):
        c = [f"GC {gc:08X} {sig}", f"source file: {src or '?'} | match: {method} ({conf:.2f})"]
        if re_name: c.append(f"nfsu2-re: {re_name}" + (f" -- {re_notes}" if re_notes else ""))
        for k, v, s in db.execute("SELECT key, value, source FROM facts WHERE subject_kind='pc_func' AND subject=?",
                                  (f"{pc:X}",)):
            c.append(f"{k}: {v} [{s}]")
        rows.append(("func", f"{pc:X}", name, clean("\n".join(c))))
    # PC-only functions identified by facts (e.g. MSVC scalar deleting destructors)
    matched = {r[0] for r in db.execute("SELECT pc_addr FROM matches")}
    for subj, val, src in db.execute("SELECT subject, value, source FROM facts WHERE subject_kind='pc_func' AND key='identity'"):
        if int(subj, 16) in matched: continue
        rows.append(("func", subj, val.split(" (")[0].split(";")[0].strip(), clean(f"{val} [{src}]")))
    # PC functions without a GameCube twin: nfsu2-re's names where they are descriptive
    import re as _re
    have = {int(r[1], 16) for r in rows}
    for pc, name, sig, notes in db.execute("SELECT addr, name, signature, notes FROM re_functions"):
        if pc in have or not name: continue
        if name.startswith("?") or name.lower().startswith("unnamed") or _re.search(r"[0-9A-F]{6}$", name): continue
        nm = _re.sub(r"[^A-Za-z0-9_:]", "_", name)
        rows.append(("func", f"{pc:X}", nm, clean(f"nfsu2-re: {sig}" + (f"\n{notes}" if notes else ""))))
    for pc, gc, name, method in db.execute("""
        SELECT m.pc_addr, m.gc_addr, g.name, m.method FROM global_matches m JOIN gc_globals g ON g.addr = m.gc_addr
        WHERE m.confidence >= ?""", (MIN_CONF,)):
        rows.append(("label", f"{pc:X}", name, clean(f"GC {gc:08X} {name} | match: {method}")))
    with open(OUT, "w") as f:
        for r in rows:
            f.write("\t".join(r) + "\n")
    print(len(rows), "entries ->", OUT)


if __name__ == "__main__":
    main()

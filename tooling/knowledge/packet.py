#!/usr/bin/env python3
"""Work packet for rewriting a function: everything known about it, in one markdown document.

Usage: packet.py <GameCube name | GC address (8 hex, 80xxxxxx) | PC address (6 hex, 4xxxxx-7xxxxx)> ...
       packet.py --class FEWideString          (every function of a class)
Prints: GameCube + PC decompilation, match (method/confidence), signatures, nfsu2-re notes, facts, the status
of every callee (rewritten / hooked / matched / unknown), callers count, rewrite status.
"""
import os, re, sqlite3, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
db = sqlite3.connect(os.path.join(ROOT, "knowledge", "knowledge.db"))
q = lambda sql, *a: db.execute(sql, a).fetchall()


def decomp(folder, file, addr, width):
    if not file: return None
    path = os.path.join(ROOT, folder, "src", file)
    if not os.path.exists(path): return None
    text = open(path, errors="replace").read()
    m = re.search(rf"^// \S+ @ {addr:0{width}x}\n(.*?)(?=^// \S+ @ [0-9a-f]+\n|\Z)", text, re.S | re.M)
    return m.group(1).strip() if m else None


def resolve(arg):
    if re.fullmatch(r"(0x)?80[0-9a-fA-F]{6}", arg):
        return [int(arg, 16)]
    if re.fullmatch(r"(0x)?[4-9][0-9a-fA-F]{5}", arg):
        r = q("SELECT gc_addr FROM matches WHERE pc_addr=?", int(arg, 16))
        if r: return [r[0][0]]
        return [-int(arg, 16)]  # PC-only function
    return [a for a, in q("SELECT addr FROM gc_functions WHERE full_name=? OR name=? ORDER BY addr", arg, arg)]


def callee_status(gc):
    rw = q("SELECT status, file FROM rewrite WHERE gc_addr=?", gc)
    if rw: return f"{rw[0][0]} in {rw[0][1]}"
    m = q("SELECT pc_addr, method, confidence FROM matches WHERE gc_addr=?", gc)
    if m: return f"not rewritten; PC {m[0][0]:06X} ({m[0][1]} {m[0][2]:.2f}) -> call through bridge"
    f = q("SELECT value FROM facts WHERE subject_kind='gc_func' AND subject=? AND key='pc_inlined'", f"{gc:08X}")
    if f: return "inlined on PC (rewrite it as part of the caller's class)"
    return "not rewritten, NO PC match (verify by reading the PC caller)"


def packet(gc):
    out = []
    if gc < 0:
        pc = -gc
        row = q("SELECT full_name, signature, decomp_file FROM pc_functions WHERE addr=?", pc)
        out.append(f"# PC-only {pc:06X} {row[0][0] if row else ''}\n")
        out.append(f"```c\n{decomp('pc_decomp', row[0][2] if row else None, pc, 8) or '(not exported)'}\n```")
        return "\n".join(out)
    g = q("SELECT full_name, signature, size, src_file, decomp_file, namespace FROM gc_functions WHERE addr=?", gc)
    if not g: return f"# {gc:08X}: unknown GameCube function"
    name, sig, size, src, dfile, ns = g[0]
    out.append(f"# {name}  (GC {gc:08X}, {size} bytes, {src or 'source file unknown'})\n")
    rw = q("SELECT status, file, note FROM rewrite WHERE gc_addr=?", gc)
    out.append(f"**Rewrite status:** {rw[0][0]} — {rw[0][1]} — {rw[0][2]}" if rw else "**Rewrite status:** not started")
    m = q("SELECT pc_addr, method, confidence, source, note FROM matches WHERE gc_addr=?", gc)
    if m:
        pc, method, conf, source, note = m[0]
        out.append(f"**PC match:** {pc:06X} via {method} (confidence {conf:.2f}, {source}) — {note}"
                   + ("  ⚠ low confidence: verify by reading both versions" if conf < 0.75 and source != "manual" else ""))
    else:
        pc = None
        out.append("**PC match:** none — the function may be inlined on PC, or unmatched (find it via the PC callers)")
    for k, v, s in q("SELECT key, value, source FROM facts WHERE subject_kind='gc_func' AND subject=?", f"{gc:08X}"):
        out.append(f"- fact `{k}`: {v} [{s}]")
    if pc:
        for k, v, s in q("SELECT key, value, source FROM facts WHERE subject_kind='pc_func' AND subject=?", f"{pc:X}"):
            out.append(f"- PC fact `{k}`: {v} [{s}]")
        r = q("SELECT name, signature, notes FROM re_functions WHERE addr=?", pc)
        if r: out.append(f"- nfsu2-re: `{r[0][1]}`" + (f" — {r[0][2]}" if r[0][2] else ""))
    out.append(f"\n## GameCube decompilation (names/types reliable; GCC 2.95 ABI)\n```c\n{decomp('gc_decomp', dfile, gc, 8) or '(missing)'}\n```")
    if pc:
        pf = q("SELECT decomp_file FROM pc_functions WHERE addr=?", pc)
        out.append(f"\n## PC decompilation (THE behavior to reproduce; MSVC x86)\n```c\n{decomp('pc_decomp', pf[0][0] if pf else None, pc, 8) or '(missing)'}\n```")
    cal = q("SELECT DISTINCT c.callee, f.full_name FROM gc_calls c JOIN gc_functions f ON f.addr=c.callee WHERE c.caller=? ORDER BY f.full_name", gc)
    if cal:
        out.append("\n## Callees (GameCube)")
        for a, n in cal: out.append(f"- `{n}` (GC {a:08X}): {callee_status(a)}")
    n_callers = q("SELECT count(DISTINCT caller) FROM gc_calls WHERE callee=?", gc)[0][0]
    out.append(f"\nCalled from {n_callers} GameCube function(s).")
    if ns:
        t = q("SELECT path, size, detail FROM gc_types WHERE name=? AND kind='struct' ORDER BY length(path) LIMIT 1", ns.split("::")[-1])
        if t: out.append(f"\n## GameCube layout of {ns} ({t[0][0]}, 0x{t[0][1]:X} bytes; PC layout may differ — check pc_decomp/types.h)\n```\n{t[0][2]}\n```")
    return "\n".join(out)


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__); sys.exit(1)
    if args[0] == "--class":
        addrs = [a for a, in q("SELECT addr FROM gc_functions WHERE namespace=? ORDER BY addr", args[1])]
    else:
        addrs = [a for x in args for a in resolve(x)]
    if not addrs:
        print("nothing found"); sys.exit(1)
    print("\n\n---\n\n".join(packet(a) for a in addrs))


if __name__ == "__main__":
    main()

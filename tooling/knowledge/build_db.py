#!/usr/bin/env python3
"""Builds/refreshes knowledge/knowledge.db, the unified NFSU2 knowledge base.

Imported (re-runnable; these tables are rebuilt each run):
  gc_*      GameCube Speed.elf, from the Ghidra export (knowledge/raw/gc) + ELF source-file ranges
  pc_*      PC SPEED2.EXE, from the Ghidra export (knowledge/raw/pc), if present
  re_*      yugecin/nfsu2-re docs (PC names/structs/notes), if a checkout path is given

Curated (never wiped by this script):
  matches   GameCube <-> PC function pairs
  facts     free-form knowledge about anything (user notes, findings), with source + confidence
  rewrite   rewrite progress per function

Usage: build_db.py [--nfsu2-re PATH]
"""
import argparse, html, json, os, re, sqlite3, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
KDIR = os.path.join(ROOT, "knowledge")
DB = os.path.join(KDIR, "knowledge.db")

SCHEMA = """
CREATE TABLE IF NOT EXISTS gc_functions(addr INTEGER PRIMARY KEY, name TEXT, full_name TEXT, namespace TEXT,
  size INTEGER, signature TEXT, callconv TEXT, sig_source TEXT, name_source TEXT, nparams INTEGER,
  thunk INTEGER, src_file TEXT, decomp_file TEXT);
CREATE TABLE IF NOT EXISTS gc_calls(caller INTEGER, callee INTEGER);
CREATE TABLE IF NOT EXISTS gc_strings(func INTEGER, string TEXT);
CREATE TABLE IF NOT EXISTS gc_features(func INTEGER PRIMARY KEY, cbr INTEGER, loops INTEGER, calls INTEGER, ops INTEGER);
CREATE TABLE IF NOT EXISTS gc_intconsts(func INTEGER, value INTEGER);
CREATE TABLE IF NOT EXISTS gc_callseq(func INTEGER, idx INTEGER, callee INTEGER);
CREATE TABLE IF NOT EXISTS gc_datarefs(func INTEGER, idx INTEGER, addr INTEGER);
CREATE TABLE IF NOT EXISTS gc_globals(addr INTEGER PRIMARY KEY, name TEXT, type TEXT, size INTEGER, string TEXT,
  nrefs INTEGER, source TEXT);
CREATE TABLE IF NOT EXISTS gc_types(path TEXT PRIMARY KEY, name TEXT, kind TEXT, size INTEGER, detail TEXT);
CREATE TABLE IF NOT EXISTS gc_src_files(start INTEGER PRIMARY KEY, file TEXT);

CREATE TABLE IF NOT EXISTS pc_functions(addr INTEGER PRIMARY KEY, name TEXT, full_name TEXT, namespace TEXT,
  size INTEGER, signature TEXT, callconv TEXT, sig_source TEXT, name_source TEXT, nparams INTEGER, thunk INTEGER,
  decomp_file TEXT);
CREATE TABLE IF NOT EXISTS pc_calls(caller INTEGER, callee INTEGER);
CREATE TABLE IF NOT EXISTS pc_strings(func INTEGER, string TEXT);
CREATE TABLE IF NOT EXISTS pc_features(func INTEGER PRIMARY KEY, cbr INTEGER, loops INTEGER, calls INTEGER, ops INTEGER);
CREATE TABLE IF NOT EXISTS pc_intconsts(func INTEGER, value INTEGER);
CREATE TABLE IF NOT EXISTS pc_callseq(func INTEGER, idx INTEGER, callee INTEGER);
CREATE TABLE IF NOT EXISTS pc_datarefs(func INTEGER, idx INTEGER, addr INTEGER);
CREATE TABLE IF NOT EXISTS pc_globals(addr INTEGER PRIMARY KEY, name TEXT, type TEXT, size INTEGER, string TEXT,
  nrefs INTEGER, source TEXT);

CREATE TABLE IF NOT EXISTS re_functions(addr INTEGER PRIMARY KEY, name TEXT, signature TEXT, notes TEXT);
CREATE TABLE IF NOT EXISTS re_globals(addr INTEGER PRIMARY KEY, name TEXT, decl TEXT, notes TEXT);
CREATE TABLE IF NOT EXISTS re_structs(name TEXT PRIMARY KEY, size INTEGER, body TEXT);

CREATE TABLE IF NOT EXISTS matches(gc_addr INTEGER, pc_addr INTEGER, method TEXT, confidence REAL,
  source TEXT, note TEXT, created TEXT DEFAULT CURRENT_TIMESTAMP, UNIQUE(gc_addr, pc_addr));
CREATE TABLE IF NOT EXISTS global_matches(gc_addr INTEGER, pc_addr INTEGER, method TEXT, confidence REAL,
  source TEXT, note TEXT, created TEXT DEFAULT CURRENT_TIMESTAMP, UNIQUE(gc_addr, pc_addr));
CREATE TABLE IF NOT EXISTS facts(id INTEGER PRIMARY KEY, subject_kind TEXT, subject TEXT, key TEXT, value TEXT,
  source TEXT, confidence REAL DEFAULT 1.0, created TEXT DEFAULT CURRENT_TIMESTAMP);
CREATE TABLE IF NOT EXISTS rewrite(gc_addr INTEGER PRIMARY KEY, pc_addr INTEGER, status TEXT, file TEXT, note TEXT,
  updated TEXT DEFAULT CURRENT_TIMESTAMP);

CREATE INDEX IF NOT EXISTS gc_calls_caller ON gc_calls(caller);
CREATE INDEX IF NOT EXISTS gc_calls_callee ON gc_calls(callee);
CREATE INDEX IF NOT EXISTS gc_strings_s ON gc_strings(string);
CREATE INDEX IF NOT EXISTS gc_fn_name ON gc_functions(full_name);
CREATE INDEX IF NOT EXISTS pc_calls_caller ON pc_calls(caller);
CREATE INDEX IF NOT EXISTS pc_calls_callee ON pc_calls(callee);
CREATE INDEX IF NOT EXISTS pc_strings_s ON pc_strings(string);
CREATE INDEX IF NOT EXISTS gc_datarefs_a ON gc_datarefs(addr);
CREATE INDEX IF NOT EXISTS pc_datarefs_a ON pc_datarefs(addr);
CREATE INDEX IF NOT EXISTS facts_subject ON facts(subject_kind, subject);

CREATE VIEW IF NOT EXISTS v_matched AS
  SELECT m.gc_addr, printf('%08X', m.gc_addr) AS gc_hex, g.full_name AS gc_name, g.signature AS gc_signature,
         m.pc_addr, printf('%06X', m.pc_addr) AS pc_hex, r.name AS re_name, m.method, m.confidence
  FROM matches m LEFT JOIN gc_functions g ON g.addr = m.gc_addr LEFT JOIN re_functions r ON r.addr = m.pc_addr;
"""


def jsonl(path):
    with open(path, encoding="utf-8") as f:
        for line in f:
            if line.strip():
                yield json.loads(line)


def elf_source_files(path):
    """(text_start, file) for each object file, from FILE symbols + their gcc2_compiled. markers."""
    d = open(path, "rb").read()
    shoff, = struct.unpack(">I", d[0x20:0x24])
    shentsize, shnum = struct.unpack(">HH", d[0x2E:0x32])
    secs = [struct.unpack(">IIIIIIIIII", d[shoff + i * shentsize: shoff + i * shentsize + 40]) for i in range(shnum)]
    symtab = next(s for s in secs if s[1] == 2)
    strtab = secs[symtab[6]]
    def name(o):
        o += strtab[4]
        return d[o:d.index(b"\0", o)].decode("latin-1")
    out, cur = [], None
    for i in range(symtab[5] // 16):
        nm, val, size, info, other, shn = struct.unpack(">IIIBBH", d[symtab[4] + i * 16: symtab[4] + i * 16 + 16])
        if info & 0xF == 4:  # STT_FILE
            cur = name(nm)
        elif cur and val and (name(nm) == "gcc2_compiled." or (info >> 4 == 0 and info & 0xF == 2)):
            out.append((val, cur))  # per-file marker, or a file-local (static) function
    # keep only the first anchor of each run of the same file
    res, last = [], None
    for val, f in sorted(set(out)):
        if f != last: res.append((val, f)); last = f
    return res


def load_program(db, prefix, raw):
    for t in ("functions", "calls", "strings", "globals", "callseq", "datarefs"):
        db.execute(f"DELETE FROM {prefix}_{t}")
    fns = []
    for o in jsonl(os.path.join(raw, "functions.jsonl")):
        fns.append(o)
        db.execute(f"INSERT INTO {prefix}_functions(addr,name,full_name,namespace,size,signature,callconv,sig_source,"
                   "name_source,nparams,thunk) VALUES(?,?,?,?,?,?,?,?,?,?,?)",
                   (o["addr"], o["name"], o["full_name"], o["namespace"], o["size"], o["signature"], o["callconv"],
                    o["sig_source"], o["name_source"], o["nparams"], int(o["thunk"])))
        db.executemany(f"INSERT INTO {prefix}_calls VALUES(?,?)", [(o["addr"], c) for c in o["callees"]])
        db.executemany(f"INSERT INTO {prefix}_strings VALUES(?,?)", [(o["addr"], s) for s in o["strings"]])
        db.executemany(f"INSERT INTO {prefix}_callseq VALUES(?,?,?)", [(o["addr"], i, c) for i, c in enumerate(o.get("call_seq", []))])
        db.executemany(f"INSERT INTO {prefix}_datarefs VALUES(?,?,?)", [(o["addr"], i, a) for i, a in enumerate(o.get("data_refs", []))])
    for o in jsonl(os.path.join(raw, "globals.jsonl")):
        db.execute(f"INSERT OR REPLACE INTO {prefix}_globals VALUES(?,?,?,?,?,?,?)",
                   (o["addr"], o["name"], o.get("type"), o.get("size"), o.get("string"), o["nrefs"], o["source"]))
    feats = os.path.join(raw, "features.jsonl")
    if os.path.exists(feats):
        db.execute(f"DELETE FROM {prefix}_features"); db.execute(f"DELETE FROM {prefix}_intconsts")
        for o in jsonl(feats):
            db.execute(f"INSERT OR REPLACE INTO {prefix}_features VALUES(?,?,?,?,?)",
                       (o["addr"], o["cbr"], o["loops"], o["calls"], o["ops"]))
            db.executemany(f"INSERT INTO {prefix}_intconsts VALUES(?,?)", [(o["addr"], v) for v in o["consts"]])
    return fns


def load_gc(db):
    raw = os.path.join(KDIR, "raw", "gc")
    if not os.path.exists(os.path.join(raw, "functions.jsonl")):
        print("gc: no export, skipped"); return
    load_program(db, "gc", raw)
    db.execute("DELETE FROM gc_types")
    for o in jsonl(os.path.join(raw, "types.jsonl")):
        detail = o.get("fields") or o.get("values") or o.get("base")
        db.execute("INSERT OR REPLACE INTO gc_types VALUES(?,?,?,?,?)",
                   (o["path"], o["name"], o["kind"], o["size"], json.dumps(detail) if detail is not None else None))
    # original source file of each function
    db.execute("DELETE FROM gc_src_files")
    files = elf_source_files(os.path.join(ROOT, "Speed.elf"))
    db.executemany("INSERT OR REPLACE INTO gc_src_files VALUES(?,?)", files)
    db.execute("""UPDATE gc_functions SET src_file =
        (SELECT file FROM gc_src_files s WHERE s.start <= gc_functions.addr ORDER BY s.start DESC LIMIT 1)""")
    # Nintendo SDK libraries have no FILE symbols, so range-based assignment is wrong for them
    sdk = re.compile(r"^_*(OS|AR|ARQ|CARD|DVD|GX|VI|PAD|SI|EXI|AI|AX|AXFX|DSP|MTX|PSMTX|PSVEC|PSQUAT|C_MTX|C_VEC|C_QUAT"
                     r"|DB|TRK|PPC|LC|THP|DTK|GBA|SIS?|EXI2|Dsp|__ar|__dsp|__DSP|__OS|__GX|__CARD|__DVD|__VI|__PAD|__SI"
                     r"|__EXI|__AI|__AX|__AR)[A-Z_a-z0-9]")
    rows = db.execute("SELECT addr, name FROM gc_functions WHERE namespace = ''").fetchall()
    db.executemany("UPDATE gc_functions SET src_file='(Nintendo SDK)' WHERE addr=?",
                   [(a,) for a, n in rows if sdk.match(n)])
    # C++ class members can't come from a C file: those sit in a stretch without file markers
    db.execute("UPDATE gc_functions SET src_file=NULL WHERE namespace<>'' AND src_file LIKE '%.c'")
    # where the function's decompiled code is in decomp/src
    idx = os.path.join(ROOT, "gc_decomp", "functions.tsv")
    if os.path.exists(idx):
        with open(idx) as f:
            next(f)
            db.executemany("UPDATE gc_functions SET decomp_file=? WHERE addr=?",
                           [(l.split("\t")[1], int(l.split("\t")[0], 16)) for l in f if l.strip()])
    print("gc:", db.execute("SELECT count(*) FROM gc_functions").fetchone()[0], "functions,",
          len(files), "source files")


def load_pc(db):
    raw = os.path.join(KDIR, "raw", "pc")
    if not os.path.exists(os.path.join(raw, "functions.jsonl")):
        print("pc: no export yet, skipped"); return
    load_program(db, "pc", raw)
    idx = os.path.join(ROOT, "pc_decomp", "functions.tsv")
    if os.path.exists(idx):
        with open(idx) as f:
            next(f)
            db.executemany("UPDATE pc_functions SET decomp_file=? WHERE addr=?",
                           [(l.split("\t")[1], int(l.split("\t")[0], 16)) for l in f if l.strip()])
    print("pc:", db.execute("SELECT count(*) FROM pc_functions").fetchone()[0], "functions")


def text(h):
    return html.unescape(re.sub(r"<[^>]+>", "", h)).strip()


def load_nfsu2_re(db, path):
    docs = os.path.join(path, "docs")
    for t in ("functions", "globals", "structs"):
        db.execute(f"DELETE FROM re_{t}")
    entry = re.compile(r"<a id=([0-9A-F]+)></a>\s*(.*?)(?=<a id=[0-9A-F]+></a>|\Z)", re.S)
    s = open(os.path.join(docs, "funcs.html"), encoding="utf-8", errors="replace").read()
    for addr, body in entry.findall(s):
        m = re.search(r"<div><i>[0-9A-F]+</i>(.*?)</div>", body, re.S)
        if not m: continue
        sig = text(m.group(1))
        nm = re.search(r"<h3>(.*?)</h3>", m.group(1), re.S)
        notes = " ".join(text(x) for x in re.findall(r"<div class=mm>(.*?)</div>", body, re.S)) or None
        db.execute("INSERT OR REPLACE INTO re_functions VALUES(?,?,?,?)",
                   (int(addr, 16), text(nm.group(1)) if nm else None, sig, notes))
    s = open(os.path.join(docs, "vars.html"), encoding="utf-8", errors="replace").read()
    for addr, body in entry.findall(s):
        m = re.search(r"<div><i>[0-9A-F]+</i>(.*?)</div>", body, re.S)
        if not m: continue
        nm = re.search(r"<h3>(.*?)</h3>", m.group(1), re.S)
        notes = " ".join(text(x) for x in re.findall(r"<div class=mm>(.*?)</div>", body, re.S)) or None
        db.execute("INSERT OR REPLACE INTO re_globals VALUES(?,?,?,?)",
                   (int(addr, 16), text(nm.group(1)) if nm else None, text(m.group(1)), notes))
    s = open(os.path.join(docs, "structs.html"), encoding="utf-8", errors="replace").read()
    for nm, body in re.findall(r"<a id=struc_([^>]+)></a>\s*<div>(.*?)</div>", s, re.S):
        size = re.search(r"size ([0-9A-F]+)h", body)
        db.execute("INSERT OR REPLACE INTO re_structs VALUES(?,?,?)",
                   (nm, int(size.group(1), 16) if size else None, text(body)))
    print("nfsu2-re:", *(f"{db.execute(f'SELECT count(*) FROM re_{t}').fetchone()[0]} {t}"
                         for t in ("functions", "globals", "structs")))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--nfsu2-re", help="path to a yugecin/nfsu2-re checkout")
    a = ap.parse_args()
    os.makedirs(KDIR, exist_ok=True)
    db = sqlite3.connect(DB)
    db.executescript(SCHEMA)
    cols = {r[1] for r in db.execute("PRAGMA table_info(pc_functions)")}
    if "decomp_file" not in cols: db.execute("ALTER TABLE pc_functions ADD COLUMN decomp_file TEXT")
    load_gc(db)
    load_pc(db)
    if a.nfsu2_re:
        load_nfsu2_re(db, a.nfsu2_re)
    db.commit()
    print("db:", DB)


if __name__ == "__main__":
    main()

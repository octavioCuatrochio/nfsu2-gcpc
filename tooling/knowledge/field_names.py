#!/usr/bin/env python3
"""Field knowledge for PC class layouts -> knowledge/raw/transfer/fields.tsv (for ApplyFieldKnowledge.java).

nfsu2-re documents PC structs under its own names; matched methods tell which GameCube class each one is
(nfsu2-re `struct UILabel *this` on a function matched to GC FEString::SetString => UILabel is FEString).
Their fields are at PC (MSVC) offsets, the same offsets as our converted layouts, so names and notes are
applied by offset. User facts (subject_kind='field', subject='Class+0xOFF') override everything.
Rows: struct<TAB>offset<TAB>name<TAB>ctype<TAB>comment
"""
import collections, os, re, sqlite3

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DB = os.path.join(ROOT, "knowledge", "knowledge.db")
OUT = os.path.join(ROOT, "knowledge", "raw", "transfer", "fields.tsv")
GENERIC = re.compile(r"^(field_|_pad|pad|unk|unknown|_unk)[0-9A-Fa-fx_]*$")


def struct_mapping(db):
    votes = collections.defaultdict(collections.Counter)
    for ns, resig in db.execute("""SELECT g.namespace, r.signature FROM matches m JOIN gc_functions g ON g.addr=m.gc_addr
                                   JOIN re_functions r ON r.addr=m.pc_addr WHERE g.namespace<>'' AND m.confidence>=0.6"""):
        mm = re.search(r"struct (\w+) \*this", resig or "")
        if mm: votes[mm.group(1)][ns] += 1
    out = {}
    for re_s, v in votes.items():
        (ns, k), = v.most_common(1)
        if k >= 2 * (sum(v.values()) - k) and (k >= 2 or sum(v.values()) == 1):
            out[re_s] = ns
    return out


def parse_re_struct(body):
    """yields (offset, ctype, name, note) from nfsu2-re's struct text"""
    note = None
    for line in body.splitlines():
        line = line.strip()
        m = re.match(r"/\*\*(.*)\*/$", line)
        if m:
            note = m.group(1).strip(); continue
        m = re.match(r"/\*([0-9A-Fa-f]+)\*/\s*(.+?)\s*([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*;", line)
        if m:
            off, ctype, name, arr = int(m.group(1), 16), m.group(2).strip(), m.group(3), m.group(4) or ""
            ctype = ctype.replace("{", "").replace("}", "").strip()
            yield off, ctype + arr, name, note
            note = None


def main():
    db = sqlite3.connect(DB)
    mapping = struct_mapping(db)
    rows = []
    bodies = dict(db.execute("SELECT name, body FROM re_structs"))
    for re_name, cls in sorted(mapping.items()):
        body = bodies.get(re_name)
        if not body: continue
        for off, ctype, name, note in parse_re_struct(body):
            if GENERIC.match(name) and not note: continue
            comment = f"nfsu2-re {re_name}.{name}" + (f": {note}" if note else "")
            rows.append((f"/Demangler/{cls}", off, "" if GENERIC.match(name) else name, ctype, comment))
    for subj, val, src in db.execute("SELECT subject, value, source FROM facts WHERE subject_kind='field' AND key='name'"):
        cls, off = subj.split("+")
        rows.append((f"/Demangler/{cls}", int(off, 16), val, "", f"[{src}]"))
    with open(OUT, "w") as f:
        f.write("".join("\t".join(map(str, r)).replace("\n", " ") + "\n" for r in rows))
    print(f"{len(mapping)} nfsu2-re structs mapped to GC classes, {len(rows)} field rows -> {OUT}")


if __name__ == "__main__":
    main()

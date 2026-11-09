#!/usr/bin/env python3
"""Matches GameCube functions (and globals) to PC ones and stores the pairs in knowledge.db.

Seed:
  strings    a string referenced by exactly one function in each program
Then repeated until nothing new is found:
  callgraph  an unmatched neighbour (callee/caller) that is the only candidate, or the only one with
             the same already-matched neighbourhood on both sides
  callseq    align the ordered call lists of each matched pair on their matched calls; unmatched calls
             in equal-length gaps between two anchors pair up by position (needs agreement of all votes)
  globals    a global accessed by matched functions pairs with the PC global that the same matched
             functions access; a function that is the only unmatched user of a matched global on both
             sides is matched through it

Pairs with source 'user' or 'manual' are kept and never overridden; 'auto' pairs are recomputed each run.
"""
import os, sqlite3, collections, struct

DB = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "knowledge", "knowledge.db")
MIN_LEN = 6  # ignore short strings like "%d", "rb"


class Matcher:
    def __init__(self, db):
        self.db = db
        db.execute("DELETE FROM matches WHERE source='auto'")
        db.execute("DELETE FROM global_matches WHERE source='auto'")
        self.g2p, self.p2g = {}, {}
        for g, p in db.execute("SELECT gc_addr, pc_addr FROM matches"):
            self.g2p[g] = p; self.p2g[p] = g
        self.gg2p, self.gp2g = {}, {}
        for g, p in db.execute("SELECT gc_addr, pc_addr FROM global_matches"):
            self.gg2p[g] = p; self.gp2g[p] = g
        self.counts = collections.Counter()
        L = lambda q: db.execute(q).fetchall()
        def graph(prefix):
            out, inn = collections.defaultdict(set), collections.defaultdict(set)
            for a, b in L(f"SELECT caller, callee FROM {prefix}_calls"):
                out[a].add(b); inn[b].add(a)
            return out, inn
        self.gout, self.ginn = graph("gc")
        self.pout, self.pinn = graph("pc")
        def seqs(prefix, table, col):
            d = collections.defaultdict(list)
            for f, x in L(f"SELECT func, {col} FROM {prefix}_{table} ORDER BY func, idx"):
                d[f].append(x)
            return d
        self.gseq, self.pseq = seqs("gc", "callseq", "callee"), seqs("pc", "callseq", "callee")
        self.gdata, self.pdata = seqs("gc", "datarefs", "addr"), seqs("pc", "datarefs", "addr")
        self.gusers, self.pusers = collections.defaultdict(set), collections.defaultdict(set)
        for f, refs in self.gdata.items():
            for a in refs: self.gusers[a].add(f)
        for f, refs in self.pdata.items():
            for a in refs: self.pusers[a].add(f)
        self.gstr, self.pstr = collections.defaultdict(set), collections.defaultdict(set)
        for f, s in L("SELECT func, string FROM gc_strings"): self.gstr[f].add(s)
        for f, s in L("SELECT func, string FROM pc_strings"): self.pstr[f].add(s)
        self.gthunk = {a for a, in L("SELECT addr FROM gc_functions WHERE thunk")}
        self.pthunk = {a for a, in L("SELECT addr FROM pc_functions WHERE thunk")}
        self.gint, self.pint = collections.defaultdict(set), collections.defaultdict(set)
        for f, v in L("SELECT func, value FROM gc_intconsts"): self.gint[f].add(v)
        for f, v in L("SELECT func, value FROM pc_intconsts"): self.pint[f].add(v)
        self.gfeat = {r[0]: r[1:] for r in L("SELECT * FROM gc_features")}
        self.pfeat = {r[0]: r[1:] for r in L("SELECT * FROM pc_features")}
        # only rare integer constants are evidence (0x100, 0x1000, ... appear everywhere)
        gfreq = collections.Counter(v for vs in self.gint.values() for v in vs)
        pfreq = collections.Counter(v for vs in self.pint.values() for v in vs)
        RARE = int(os.environ.get("MATCH_RARE_INT", 40))
        rare = lambda v: gfreq[v] <= RARE and pfreq[v] <= RARE
        self.gint = collections.defaultdict(set, {f: {v for v in vs if rare(v)} for f, vs in self.gint.items()})
        self.pint = collections.defaultdict(set, {f: {v for v in vs if rare(v)} for f, vs in self.pint.items()})
        # GameCube-only code (Nintendo SDK) has no PC counterpart: never a match candidate
        self.gfuncs = {a for a, in L("SELECT addr FROM gc_functions WHERE ifnull(src_file,'') <> '(Nintendo SDK)'")}
        self.pfuncs = {a for a, in L("SELECT addr FROM pc_functions")}

    # ---------------------------------------------------------------- helpers
    def add(self, g, p, method, conf, note):
        if g in self.g2p or p in self.p2g or g not in self.gfuncs or p not in self.pfuncs:
            return False
        if not self.compatible(g, p):
            return False
        self.g2p[g] = p; self.p2g[p] = g
        self.db.execute("INSERT OR IGNORE INTO matches(gc_addr,pc_addr,method,confidence,source,note) VALUES(?,?,?,?,?,?)",
                        (g, p, method, conf, "auto", note))
        self.counts[method] += 1
        return True

    def add_global(self, g, p, method, conf, note):
        if g in self.gg2p or p in self.gp2g:
            return False
        self.gg2p[g] = p; self.gp2g[p] = g
        self.db.execute("INSERT OR IGNORE INTO global_matches(gc_addr,pc_addr,method,confidence,source,note) VALUES(?,?,?,?,?,?)",
                        (g, p, method, conf, "auto", note))
        self.counts["global:" + method] += 1
        return True

    def compatible(self, g, p):
        """No contradicting evidence: shared strings if both have strings, matched callees present on PC."""
        if self.gstr[g] and self.pstr[p] and not (self.gstr[g] & self.pstr[p]):
            return False
        bad = sum(1 for c in self.gout[g] if c in self.g2p and self.g2p[c] not in self.pout[p])
        good = sum(1 for c in self.gout[g] if c in self.g2p and self.g2p[c] in self.pout[p])
        return bad <= good  # allow some inlining differences, but not a majority

    # ---------------------------------------------------------------- passes
    def seed_strings(self):
        def owners(prefix):
            o = collections.defaultdict(set)
            for f, s in self.db.execute(f"SELECT func, string FROM {prefix}_strings"):
                if len(s) >= MIN_LEN: o[s].add(f)
            return {s: next(iter(fs)) for s, fs in o.items() if len(fs) == 1}
        gs, ps = owners("gc"), owners("pc")
        votes = collections.defaultdict(collections.Counter)
        for s in gs.keys() & ps.keys():
            votes[gs[s]][ps[s]] += 1
        claims = collections.Counter(p for v in votes.values() for p in v)
        for g, v in sorted(votes.items()):
            (p, k), = sorted(v.items(), key=lambda x: (-x[1], x[0]))[:1]
            if sum(v.values()) == k and claims[p] == 1:
                self.add(g, p, "strings", min(0.99, 0.8 + 0.05 * k), f"{k} unique shared string(s)")

    def callgraph(self):
        added = 0
        for g, p in list(self.g2p.items()):
            for gnb, pnb in ((self.gout, self.pout), (self.ginn, self.pinn)):
                gc_c = [x for x in gnb[g] if x not in self.g2p and x not in self.gthunk]
                pc_c = [x for x in pnb[p] if x not in self.p2g and x not in self.pthunk]
                if not gc_c or not pc_c:
                    continue
                if len(gc_c) == 1 and len(pc_c) == 1:
                    added += self.add(gc_c[0], pc_c[0], "callgraph", 0.7, f"only unmatched neighbour of {g:08X}/{p:06X}")
                    continue
                gfp, pfp = collections.defaultdict(list), collections.defaultdict(list)
                for x in gc_c:
                    f = (frozenset(self.g2p[y] for y in self.gout[x] if y in self.g2p),
                         frozenset(self.g2p[y] for y in self.ginn[x] if y in self.g2p))
                    if f[0] or f[1]: gfp[f].append(x)
                for x in pc_c:
                    f = (frozenset(y for y in self.pout[x] if y in self.p2g), frozenset(y for y in self.pinn[x] if y in self.p2g))
                    if f[0] or f[1]: pfp[f].append(x)
                for f, xs in gfp.items():
                    if len(xs) == 1 and len(pfp.get(f, [])) == 1 and len(f[0]) + len(f[1]) >= 2:
                        added += self.add(xs[0], pfp[f][0], "callgraph", 0.85, "same matched neighbourhood")
        return added

    def callseq(self):
        votes = collections.defaultdict(collections.Counter)
        for g, p in list(self.g2p.items()):
            G, P = self.gseq.get(g), self.pseq.get(p)
            if not G or not P:
                continue
            # LCS on matched calls -> anchor positions
            Gm = [self.g2p.get(x) for x in G]
            n, m = len(G), len(P)
            if n * m > 40000:
                continue
            dp = [[0] * (m + 1) for _ in range(n + 1)]
            for i in range(n - 1, -1, -1):
                for j in range(m - 1, -1, -1):
                    dp[i][j] = dp[i + 1][j + 1] + 1 if Gm[i] is not None and Gm[i] == P[j] else max(dp[i + 1][j], dp[i][j + 1])
            anchors, i, j = [(-1, -1)], 0, 0
            while i < n and j < m:
                if Gm[i] is not None and Gm[i] == P[j]:
                    anchors.append((i, j)); i += 1; j += 1
                elif dp[i + 1][j] >= dp[i][j + 1]:
                    i += 1
                else:
                    j += 1
            anchors.append((n, m))
            if len(anchors) < 3:
                continue  # need at least one real anchor
            for (i1, j1), (i2, j2) in zip(anchors, anchors[1:]):
                gg, pp = G[i1 + 1:i2], P[j1 + 1:j2]
                if not gg or len(gg) != len(pp):
                    continue
                for a, b in zip(gg, pp):
                    if a not in self.g2p and b not in self.p2g and a not in self.gthunk and b not in self.pthunk:
                        votes[a][b] += 1
        added = 0
        claims = collections.defaultdict(set)
        for a, v in votes.items():
            for b in v: claims[b].add(a)
        for a, v in sorted(votes.items()):
            (b, k), = sorted(v.items(), key=lambda x: (-x[1], x[0]))[:1]
            if sum(v.values()) == k and len(claims[b]) == 1:
                added += self.add(a, b, "callseq", min(0.9, 0.7 + 0.05 * k), f"{k} aligned call position(s)")
        return added

    def globals_(self):
        votes = collections.defaultdict(collections.Counter)
        for g, p in self.g2p.items():
            gd = [a for a in self.gdata.get(g, []) if a not in self.gg2p]
            pd = [a for a in self.pdata.get(p, []) if a not in self.gp2g]
            if len(gd) == 1 and len(pd) == 1:
                votes[gd[0]][pd[0]] += 1
        claims = collections.defaultdict(set)
        for a, v in votes.items():
            for b in v: claims[b].add(a)
        added = 0
        for a, v in sorted(votes.items()):
            (b, k), = sorted(v.items(), key=lambda x: (-x[1], x[0]))[:1]
            if sum(v.values()) == k and len(claims[b]) == 1:
                added += self.add_global(a, b, "accessors", min(0.9, 0.6 + 0.1 * k), f"only unmatched global of {k} matched pair(s)")
        # functions via matched globals
        for a, b in list(self.gg2p.items()):
            gu = [f for f in self.gusers[a] if f not in self.g2p]
            pu = [f for f in self.pusers[b] if f not in self.p2g]
            if len(gu) == 1 and len(pu) == 1:
                added += self.add(gu[0], pu[0], "globals", 0.75, f"only unmatched user of global {a:08X}/{b:06X}")
        return added

    # ---------------------------------------------------------------- vtables
    def load_vtables(self):
        """GameCube: labeled *_virtual_table globals (GCC 2.95: 8-byte {delta,index,pfn} entries, slot 0 empty).
        PC: runs of >=2 function pointers in read-only data that code references (MSVC: 4-byte pfns from slot 0)."""
        root = os.path.dirname(os.path.dirname(DB))
        elf = open(os.path.join(root, "Speed.elf"), "rb").read()
        ph, = struct.unpack(">I", elf[0x1C:0x20]); phes, phn = struct.unpack(">HH", elf[0x2A:0x2E])
        segs = []
        for i in range(phn):
            t, off, va, pa, fs, ms, fl, al = struct.unpack(">8I", elf[ph + i * phes: ph + i * phes + 32])
            if t == 1 and fs: segs.append((va, off, fs))
        def gread(a, n):
            for va, off, fs in segs:
                if va <= a and a + n <= va + fs: return elf[off + a - va: off + a - va + n]
        self.gvt = {}
        for a, size in self.db.execute("SELECT addr, size FROM gc_globals WHERE name LIKE '%virtual_table%'"):
            b = gread(a, size or 0) if size else None
            if not b: continue
            self.gvt[a] = [struct.unpack(">hhI", b[i:i + 8])[2] for i in range(0, len(b) - 7, 8)]
        exe = open(os.path.join(root, "pc", "SPEED2.EXE"), "rb").read()
        pe, = struct.unpack("<I", exe[0x3C:0x40])
        nsec, = struct.unpack("<H", exe[pe + 6:pe + 8]); opt, = struct.unpack("<H", exe[pe + 20:pe + 22])
        base, = struct.unpack("<I", exe[pe + 24 + 28:pe + 24 + 32])
        secs = []
        for i in range(nsec):
            s = pe + 24 + opt + i * 40
            name = exe[s:s + 8].rstrip(b"\0"); vsz, va, rsz, roff = struct.unpack("<IIII", exe[s + 8:s + 24])
            flags, = struct.unpack("<I", exe[s + 36:s + 40])
            secs.append((name, base + va, rsz, roff, flags))
        referenced = {a for a, in self.db.execute("SELECT DISTINCT addr FROM pc_datarefs")}
        self.pvt = {}
        for name, va, rsz, roff, flags in secs:
            if flags & 0x20000000 or not flags & 0x40000000: continue  # skip code; keep readable data
            words = struct.unpack("<%dI" % (rsz // 4), exe[roff:roff + rsz // 4 * 4])
            for i in range(len(words)):
                a = va + 4 * i
                if a not in referenced or words[i] not in self.pfuncs: continue
                n = i
                while n < len(words) and words[n] in self.pfuncs and (n == i or va + 4 * n not in referenced):
                    n += 1
                if n - i >= 2: self.pvt[a] = list(words[i:n])
        self.pvt_by_entry = collections.defaultdict(set)
        for t, ents in self.pvt.items():
            for k, f in enumerate(ents): self.pvt_by_entry[(f, k)].add(t)
        self.vt_pairs = {}
        self.folded = set()
        self.scalar_dtors = {}
        self.gname = dict(self.db.execute("SELECT addr, full_name FROM gc_functions"))
        # pure-virtual stubs: GCC's __pure_virtual; on PC, the entry filling the most table slots (_purecall)
        self.gpure = next((a for a, in self.db.execute("SELECT addr FROM gc_functions WHERE name='__pure_virtual'")), None)
        self.ppure = None  # learned from the first paired tables (see vtables)
        print("vtables: gc", len(self.gvt), "pc candidates", len(self.pvt))

    def vtables(self):
        if not hasattr(self, "gvt"):
            self.load_vtables()
        added = 0
        for gt, gents in self.gvt.items():
            if gt in self.vt_pairs: continue
            slots = gents[1:]
            cands = collections.Counter()
            for k, f in enumerate(slots):
                if f in self.g2p:
                    for t in self.pvt_by_entry.get((self.g2p[f], k), ()): cands[t] += 1
            # constructors/destructors: matched users of the GC table point at their PC table
            ctor_votes = collections.Counter()
            for u in self.gusers.get(gt, ()):
                if u in self.g2p:
                    for a in self.pdata.get(self.g2p[u], ()):
                        if a in self.pvt: ctor_votes[a] += 1
            scored = []
            for t in set(cands) | set(ctor_votes):
                pents = self.pvt[t]
                if len(pents) < len(slots) - 4 or len(pents) > len(slots) + 8: continue
                agree = disagree = 0
                for k, f in enumerate(slots[:len(pents)]):
                    p = pents[k]
                    if f == self.gpure:
                        if self.ppure is None: continue
                        if p == self.ppure: agree += 0.5
                        else: disagree += 1
                    elif self.ppure is not None and p == self.ppure:
                        disagree += 1
                    elif f in self.g2p:
                        if self.g2p[f] == p: agree += 1
                        elif p not in self.folded: disagree += 1
                if disagree == 0 and (agree >= 2 or (ctor_votes[t] and agree >= 1)):
                    scored.append((agree + 2 * ctor_votes[t], t))
            scored.sort(reverse=True)
            if not scored or (len(scored) > 1 and scored[1][0] == scored[0][0]): continue
            t = scored[0][1]
            self.vt_pairs[gt] = t
            self.add_global(gt, t, "vtable", 0.95, f"score {scored[0][0]}")
            added += 1
        if self.ppure is None and len(self.vt_pairs) >= 5:
            c = collections.Counter(self.pvt[t][k - 1] for gt, t in self.vt_pairs.items()
                                    for k, f in enumerate(self.gvt[gt]) if k and f == self.gpure and k - 1 < len(self.pvt[t]))
            if c:
                self.ppure = c.most_common(1)[0][0]
                print("learned PC _purecall:", hex(self.ppure), dict(c.most_common(3)))
        # slot pairs from all paired tables; a PC function voted for by several different GC functions
        # is an MSVC-folded duplicate (identical code merged by the linker): never match it
        votes = collections.defaultdict(set)
        for gt, t in self.vt_pairs.items():
            for k, f in enumerate(self.gvt[gt][1:len(self.pvt[t]) + 1]):
                p = self.pvt[t][k]
                if f and f != self.gpure and p != self.ppure: votes[p].add(f)
        for p, fs in votes.items():
            if len(fs) > 1: self.folded.add(p); continue
            f, = fs
            if f in self.gthunk: continue
            name = self.gname.get(f, "")
            if "::~" in name and self.psize.get(p, 0) < 64 and self.pseq.get(p):
                # MSVC vtables hold the "scalar deleting destructor" (calls the destructor, then operator delete);
                # the GameCube slot is the destructor itself, which is what the wrapper calls first
                real = self.pseq[p][0]
                self.scalar_dtors[p] = name
                added += self.add(f, real, "vtable", 0.85, f"destructor called by scalar deleting destructor {p:06X}")
            else:
                added += self.add(f, p, "vtable", 0.9, "same vtable slot")
        return added

    # ---------------------------------------------------------------- float constants
    def load_constants(self):
        """Float/double constants each function loads from memory, read straight from both binaries."""
        import math
        root = os.path.dirname(os.path.dirname(DB))
        elf = open(os.path.join(root, "Speed.elf"), "rb").read()
        ph, = struct.unpack(">I", elf[0x1C:0x20]); phes, phn = struct.unpack(">HH", elf[0x2A:0x2E])
        gsegs = []
        for i in range(phn):
            t, off, va, pa, fs, ms, fl, al = struct.unpack(">8I", elf[ph + i * phes: ph + i * phes + 32])
            if t == 1 and fs and not fl & 1: gsegs.append((va, off, fs))  # data segments only
        exe = open(os.path.join(root, "pc", "SPEED2.EXE"), "rb").read()
        pe, = struct.unpack("<I", exe[0x3C:0x40])
        nsec, = struct.unpack("<H", exe[pe + 6:pe + 8]); opt, = struct.unpack("<H", exe[pe + 20:pe + 22])
        base, = struct.unpack("<I", exe[pe + 24 + 28:pe + 24 + 32])
        psegs = []
        for i in range(nsec):
            o = pe + 24 + opt + i * 40
            vsz, va, rsz, roff = struct.unpack("<IIII", exe[o + 8:o + 24]); flags, = struct.unpack("<I", exe[o + 36:o + 40])
            if not flags & 0x20000000: psegs.append((base + va, roff, rsz))
        def read(img, segs, a, n):
            for va, off, fs in segs:
                if va <= a and a + n <= va + fs: return img[off + a - va: off + a - va + n]
        def nice(v):
            return math.isfinite(v) and 1e-7 < abs(v) < 1e8
        def key(v):
            return "%.5g" % v
        def consts(refs, img, segs, end):
            out = set()
            for a in refs:
                b4 = read(img, segs, a, 4)
                if not b4: continue
                f, = struct.unpack(end + "f", b4)
                b8 = read(img, segs, a, 8) if a % 8 == 0 else None
                d = struct.unpack(end + "d", b8)[0] if b8 else None
                # a double's high word read as float is usually a weird value; prefer the double then
                if d is not None and nice(d) and (not nice(f) or abs(f) > 1e6):
                    out.add(key(d))
                elif nice(f):
                    out.add(key(f))
            return out
        # GameCube floats are mostly loaded r2/r13-relative (small data) or via lis+lfs; Ghidra misses many of
        # these references, so decode the float loads (lfs/lfd/lfsu/lfdu, psq_l) straight from the code
        sda = {}
        shoff, = struct.unpack(">I", elf[0x20:0x24]); shes, shn = struct.unpack(">HH", elf[0x2E:0x32])
        sh = [struct.unpack(">IIIIIIIIII", elf[shoff + i * shes: shoff + i * shes + 40]) for i in range(shn)]
        st = next(x for x in sh if x[1] == 2); strt = sh[st[6]]
        for i in range(st[5] // 16):
            nm, val = struct.unpack(">II", elf[st[4] + i * 16: st[4] + i * 16 + 8])
            o = strt[4] + nm; name = elf[o:elf.index(b"\0", o)]
            if name in (b"_SDA_BASE_", b"_SDA2_BASE_"): sda[13 if name == b"_SDA_BASE_" else 2] = val
        text_va, text_off = 0x800034A0, 0x500
        gref = collections.defaultdict(list)
        for f, size in self.db.execute("SELECT addr, size FROM gc_functions"):
            lis = {}
            o = text_off + f - text_va
            if not (0 <= o and o + size <= len(elf)): continue
            for k in range(0, size - 3, 4):
                w, = struct.unpack(">I", elf[o + k:o + k + 4])
                op, rd, ra, imm = w >> 26, (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
                simm = imm - 0x10000 if imm & 0x8000 else imm
                if op == 15 and ra == 0:  # lis rd, imm
                    lis[rd] = imm << 16; continue
                if op in (48, 49, 50, 51):  # lfs, lfsu, lfd, lfdu
                    if ra in sda: gref[f].append((sda[ra] + simm) & 0xFFFFFFFF)
                    elif ra in lis: gref[f].append((lis[ra] + simm) & 0xFFFFFFFF)
                elif op == 14 and ra in lis and rd != ra:  # addi rd, ra, lo  (address formed for a later load)
                    pass
                if op != 15 and rd in lis and op not in (48, 49, 50, 51, 52, 53, 54, 55): lis.pop(rd, None)
        self.gconst = {f: consts(set(r) | set(gref.get(f, ())), elf, gsegs, ">") for f, r in
                       list(self.gdata.items()) + [(f, []) for f in gref if f not in self.gdata]}
        self.pconst = {f: consts(r, exe, psegs, "<") for f, r in self.pdata.items()}
        self.gconst = {f: c for f, c in self.gconst.items() if c}
        self.pconst = {f: c for f, c in self.pconst.items() if c}
        print("constants: gc functions", len(self.gconst), "pc functions", len(self.pconst))

    def size_ok(self, g, p, lo=0.3, hi=3.0):
        gs, ps = self.gsize.get(g, 0), self.psize.get(p, 0)
        return gs < 32 or ps < 16 or lo <= ps / gs <= hi

    def intconstants(self):
        """Integer constants (hashes, magic numbers, sizes) used by one unmatched function per side."""
        return self._unique_values(self.gint, self.pint, "intconsts", 3)

    def _unique_values(self, gmap, pmap, method, maxfreq):
        def owners(cmap, matched):
            o = collections.defaultdict(set)
            for f, cs in cmap.items():
                if f in matched: continue
                for c in cs: o[c].add(f)
            return {c: next(iter(fs)) for c, fs in o.items() if len(fs) == 1}
        go, po = owners(gmap, self.g2p), owners(pmap, self.p2g)
        gfreq = collections.Counter(c for cs in gmap.values() for c in cs)
        pfreq = collections.Counter(c for cs in pmap.values() for c in cs)
        votes = collections.defaultdict(collections.Counter)
        for c in go.keys() & po.keys():
            if gfreq[c] <= maxfreq and pfreq[c] <= maxfreq:
                votes[go[c]][po[c]] += 1
        claims = collections.Counter(p for v in votes.values() for p in v)
        added = 0
        for g, v in sorted(votes.items()):
            (p, k), = sorted(v.items(), key=lambda x: (-x[1], x[0]))[:1]
            if sum(v.values()) != k or claims[p] != 1: continue
            # a PC function far bigger than the GC one is usually a caller that inlined it (and its constants);
            # the reverse is normal: GC loaders byte-swap their data, PC ones don't
            if not self.size_ok(g, p, 0.05, 2.5): continue
            if k >= 2 or self.size_ok(g, p, 0.4, 2.5):
                added += self.add(g, p, method, min(0.9, 0.7 + 0.05 * k), f"{k} rare shared {method} value(s)")
        return added

    def constants(self):
        """A constant value used by exactly one unmatched function on each side pairs them (like strings)."""
        def owners(cmap, matched):
            o = collections.defaultdict(set)
            for f, cs in cmap.items():
                if f in matched: continue
                for c in cs: o[c].add(f)
            return {c: next(iter(fs)) for c, fs in o.items() if len(fs) == 1}
        go, po = owners(self.gconst, self.g2p), owners(self.pconst, self.p2g)
        # values used by many functions in either program carry no information
        gfreq = collections.Counter(c for cs in self.gconst.values() for c in cs)
        pfreq = collections.Counter(c for cs in self.pconst.values() for c in cs)
        votes = collections.defaultdict(collections.Counter)
        for c in go.keys() & po.keys():
            if gfreq[c] <= 3 and pfreq[c] <= 3:
                votes[go[c]][po[c]] += 1
        claims = collections.Counter(p for v in votes.values() for p in v)
        added = 0
        for g, v in sorted(votes.items()):
            (p, k), = sorted(v.items(), key=lambda x: (-x[1], x[0]))[:1]
            if sum(v.values()) != k or claims[p] != 1: continue
            if not self.size_ok(g, p, 0.05, 2.5): continue
            if k >= 2 or self.size_ok(g, p, 0.4, 2.5):
                added += self.add(g, p, "constants", min(0.9, 0.7 + 0.05 * k), f"{k} rare shared float constant(s)")
        return added

    # ---------------------------------------------------------------- address order
    def order(self):
        """Between two consecutive matches that are in the same order on both sides and close together,
        equal numbers of unmatched functions pair up by position (sizes must be plausible)."""
        import bisect
        gl, pl = self.gorder, self.porder
        pairs = sorted(self.g2p.items())
        added = 0
        for (g1, p1), (g2, p2) in zip(pairs, pairs[1:]):
            if not (p1 < p2 <= p1 + 0x8000 and g2 - g1 <= 0x8000):
                continue
            gs = [x for x in gl[bisect.bisect_right(gl, g1):bisect.bisect_left(gl, g2)] if x not in self.gthunk]
            ps = [x for x in pl[bisect.bisect_right(pl, p1):bisect.bisect_left(pl, p2)] if x not in self.pthunk]
            if not gs or len(gs) != len(ps) or len(gs) > 12:
                continue
            if any(x in self.g2p for x in gs) or any(x in self.p2g for x in ps):
                continue
            if not all(self.size_ok(a, b) for a, b in zip(gs, ps)):
                continue
            for a, b in zip(gs, ps):
                added += self.add(a, b, "order", 0.7 if len(gs) == 1 else 0.65,
                                  f"position {gs.index(a) + 1}/{len(gs)} between {g1:08X}/{p1:06X} and {g2:08X}/{p2:06X}")
        return added

    # ---------------------------------------------------------------- multi-signal scoring
    def window_g(self, g):
        """PC address window where g should be, from the partners of its nearest matched GC neighbours."""
        import bisect
        i = bisect.bisect_left(self._mg, g)
        if i == 0 or i >= len(self._mg): return None
        a, b = self._mg[i - 1], self._mg[i]
        if b - a > 0x8000: return None
        pa, pb = self.g2p[a], self.g2p[b]
        return (pa, pb) if pa < pb <= pa + 0x8000 else None

    def window_p(self, p):
        import bisect
        i = bisect.bisect_left(self._mp, p)
        if i == 0 or i >= len(self._mp): return None
        a, b = self._mp[i - 1], self._mp[i]
        if b - a > 0x8000: return None
        ga, gb = self.p2g[a], self.p2g[b]
        return (ga, gb) if ga < gb <= ga + 0x8000 else None

    def score(self, g, p):
        """Evidence that g (GC) and p (PC) are the same function; None if contradicted or no evidence."""
        import math
        sc = 0.0
        evidence = 0
        gm_out = {self.g2p[c] for c in self.gout[g] if c in self.g2p}
        gm_in = {self.g2p[c] for c in self.ginn[g] if c in self.g2p}
        pm_out = {c for c in self.pout[p] if c in self.p2g}
        pm_in = {c for c in self.pinn[p] if c in self.p2g}
        agree = len(gm_out & self.pout[p]) + len(gm_in & self.pinn[p])
        # matched neighbours on one side that are not neighbours on the other (inlining explains a few)
        miss = len(gm_out - self.pout[p]) + len(pm_out - gm_out) + len(gm_in - self.pinn[p]) + len(pm_in - gm_in)
        sc += 2.0 * agree - 0.7 * miss
        evidence += agree
        # matched globals both functions access
        gg = {self.gg2p[a] for a in self.gdata.get(g, ()) if a in self.gg2p}
        pg = {a for a in self.pdata.get(p, ()) if a in self.gp2g}
        gagree = len(gg & pg)
        sc += 1.5 * gagree - 0.5 * len(gg ^ pg)
        evidence += gagree
        gs, ps = self.gsize.get(g, 0), self.psize.get(p, 0)
        if gs >= 32 and ps >= 16:
            r = math.log((ps / gs) / 0.75)
            if abs(r) > math.log(4): return None
            sc -= 1.5 * abs(r)
        gc, pc = len(self.gseq.get(g, ())), len(self.pseq.get(p, ()))
        sc -= 0.15 * abs(gc - pc)
        a, b = self.gconst.get(g, set()), self.pconst.get(p, set())
        if a and b:
            shared = a & b
            sc += 3.0 * len(shared) / len(a | b) - (1.0 if not shared and len(a) > 2 and len(b) > 2 else 0)
            evidence += bool(shared)
        ia, ib = self.gint.get(g, set()), self.pint.get(p, set())
        if ia and ib:
            shared = ia & ib
            sc += 4.0 * len(shared) / len(ia | ib) - (1.0 if not shared and len(ia) > 2 and len(ib) > 2 else 0)
            evidence += bool(shared)
        fa, fb = self.gfeat.get(g), self.pfeat.get(p)
        if fa and fb:
            # branches / loops / calls: same source code gives similar control flow on both compilers
            for i, w in ((0, 0.12), (1, 0.5), (2, 0.1)):
                sc -= w * abs(fa[i] - fb[i])
        if self.gstr[g] or self.pstr[p]:
            if self.gstr[g] & self.pstr[p]: sc += 3.0; evidence += 1
            elif self.gstr[g] and self.pstr[p]: return None
        w = self.window_g(g)
        if w and w[0] < p < w[1]: sc += 1.0
        if not evidence:
            return None
        return sc

    def neighbourhood(self, threshold=float(os.environ.get("MATCH_THRESHOLD", 2.5)),
                      margin=float(os.environ.get("MATCH_MARGIN", 1.0))):
        """Unmatched functions near matched ones (calls, globals, strings, rare constants, address window):
        take mutual best candidates by score()."""
        import bisect
        self._mg = sorted(self.g2p); self._mp = sorted(self.p2g)
        if not hasattr(self, "_idx"):
            def owners(m, cap):
                o = collections.defaultdict(set)
                for f, vs in m.items():
                    for v in vs: o[v].add(f)
                return {v: fs for v, fs in o.items() if len(fs) <= cap}
            self._gstr_o, self._pstr_o = owners(self.gstr, 10), owners(self.pstr, 10)
            self._gc_o, self._pc_o = owners(self.gconst, 10), owners(self.pconst, 10)
            self._gi_o, self._pi_o = owners(self.gint, 10), owners(self.pint, 10)
            self._gusers_c = {a: fs for a, fs in self.gusers.items() if len(fs) <= 30}
            self._pusers_c = {a: fs for a, fs in self.pusers.items() if len(fs) <= 30}
            self._idx = True
        def cands_for_g(g):
            out = set()
            for c in self.gout[g]:
                if c in self.g2p: out |= self.pinn[self.g2p[c]]
            for c in self.ginn[g]:
                if c in self.g2p: out |= self.pout[self.g2p[c]]
            for a in self.gdata.get(g, ()):
                if a in self.gg2p: out |= self._pusers_c.get(self.gg2p[a], set())
            for s_ in self.gstr[g]: out |= self._pstr_o.get(s_, set())
            for c in self.gconst.get(g, ()): out |= self._pc_o.get(c, set())
            for c in self.gint.get(g, ()): out |= self._pi_o.get(c, set())
            w = self.window_g(g)
            if w: out |= set(self.porder[bisect.bisect_right(self.porder, w[0]):bisect.bisect_left(self.porder, w[1])])
            return {x for x in out if x not in self.p2g and x not in self.pthunk}
        def cands_for_p(p):
            out = set()
            for c in self.pout[p]:
                if c in self.p2g: out |= self.ginn[self.p2g[c]]
            for c in self.pinn[p]:
                if c in self.p2g: out |= self.gout[self.p2g[c]]
            for a in self.pdata.get(p, ()):
                if a in self.gp2g: out |= self._gusers_c.get(self.gp2g[a], set())
            for s_ in self.pstr[p]: out |= self._gstr_o.get(s_, set())
            for c in self.pconst.get(p, ()): out |= self._gc_o.get(c, set())
            for c in self.pint.get(p, ()): out |= self._gi_o.get(c, set())
            w = self.window_p(p)
            if w: out |= set(self.gorder[bisect.bisect_right(self.gorder, w[0]):bisect.bisect_left(self.gorder, w[1])])
            return {x for x in out if x not in self.g2p and x not in self.gthunk}
        def best(scores):
            scores = sorted(((s, x) for x, s in scores.items() if s is not None), reverse=True)
            if not scores or scores[0][0] < threshold: return None
            if len(scores) > 1 and scores[0][0] - scores[1][0] < margin: return None
            return scores[0]
        proposals = []
        for g in sorted(self.gfuncs):
            if g in self.g2p or g in self.gthunk: continue
            cs = cands_for_g(g)
            if not cs or len(cs) > 300: continue
            b = best({p: self.score(g, p) for p in cs})
            if not b: continue
            s, p = b
            back = best({x: self.score(x, p) for x in cands_for_p(p)})
            if back and back[1] == g:
                proposals.append((s, g, p))
        added = 0
        for s, g, p in sorted(proposals, reverse=True):
            added += self.add(g, p, "score", min(0.85, 0.55 + 0.03 * s), f"mutual best, score {s:.1f}")
        return added

    def run(self):
        self.gsize = dict(self.db.execute("SELECT addr, size FROM gc_functions"))
        self.psize = dict(self.db.execute("SELECT addr, size FROM pc_functions"))
        self.gorder = sorted(self.gfuncs); self.porder = sorted(self.pfuncs)
        self.load_constants()
        self.seed_strings()
        for rnd in range(1, 60):
            added = self.callgraph() + self.callseq() + self.globals_() + self.vtables() + self.constants() + self.intconstants()
            if not added:
                added = self.neighbourhood()
            if not added:
                added = self.order()  # weakest signal: only once everything else has settled
            if not added:
                break
        n = self.db.execute("""UPDATE matches SET confidence = 0.4, note = note || ' [size ratio implausible]'
            WHERE source='auto' AND rowid IN (SELECT m.rowid FROM matches m
              JOIN gc_functions g ON g.addr=m.gc_addr JOIN pc_functions p ON p.addr=m.pc_addr
              WHERE g.size >= 64 AND (p.size * 1.0 / g.size < 0.1 OR p.size * 1.0 / g.size > 4))""").rowcount
        # name the MSVC scalar deleting destructors (PC-only functions) as facts
        self.db.execute("DELETE FROM facts WHERE source='match.py' AND key='identity'")
        self.db.executemany("INSERT INTO facts(subject_kind,subject,key,value,source,confidence) VALUES('pc_func',?,"
                            "'identity',?,'match.py',0.85)",
                            [(f"{p:X}", f"{n.rsplit('::', 1)[0]}::`scalar deleting destructor' (MSVC wrapper: calls {n}, then operator delete)")
                             for p, n in sorted(getattr(self, "scalar_dtors", {}).items())])
        self.db.commit()
        print(dict(self.counts), "rounds:", rnd, "flagged:", n)
        print("function matches:", len(self.g2p), "global matches:", len(self.gg2p))


if __name__ == "__main__":
    Matcher(sqlite3.connect(DB)).run()

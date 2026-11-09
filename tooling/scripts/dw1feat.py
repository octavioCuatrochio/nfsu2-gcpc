import sys, struct, collections
f = open(sys.argv[1], 'rb').read()
shoff, = struct.unpack('>I', f[0x20:0x24]); shentsize, shnum, shstrndx = struct.unpack('>HHH', f[0x2e:0x34])
secs = [struct.unpack('>IIIIIIIIII', f[shoff+i*shentsize: shoff+(i+1)*shentsize]) for i in range(shnum)]
stro = secs[shstrndx][4]
def nm(o): return f[stro+o: f.index(b'\0', stro+o)].decode()
dbg = next(s for s in secs if nm(s[0]) == '.debug'); d = f[dbg[4]: dbg[4]+dbg[5]]
dies = {}; order = []
p = 0
while p + 4 <= len(d):
    ln, = struct.unpack('>I', d[p:p+4]); end = p + max(ln,4)
    if ln < 8: dies[p] = (None, {}); order.append(p); p = end; continue
    tag, = struct.unpack('>H', d[p+4:p+6]); q = p + 6; A = {}
    while q < end:
        at, = struct.unpack('>H', d[q:q+2]); q += 2; form = at & 0xf
        if form in (1,2,6): v = struct.unpack('>I', d[q:q+4])[0]; q += 4
        elif form == 5: v = struct.unpack('>H', d[q:q+2])[0]; q += 2
        elif form == 7: v = d[q:q+8]; q += 8
        elif form == 3: n, = struct.unpack('>H', d[q:q+2]); v = d[q+2:q+2+n]; q += 2+n
        elif form == 4: n, = struct.unpack('>I', d[q:q+4]); v = d[q+4:q+4+n]; q += 4+n
        elif form == 8: e = d.index(b'\0', q); v = d[q:e].decode('latin1'); q = e+1
        A[at & 0xfff0] = v
    dies[p] = (tag, A); order.append(p); p = end
C = collections.Counter
ft = C(); modft = C(); mods = C(); arrfmt = C(); locops = C(); enumsz = C(); anon = C(); bit = 0; decl = C(); badparent = 0
def locdesc(b):
    ops = []; i = 0
    while i < len(b):
        op = b[i]; i += 1
        if op in (1,2,3,4): ops.append((op, struct.unpack('>I', b[i:i+4])[0])); i += 4
        else: ops.append((op, None))
    return ops
OPN = {1:'REG',2:'BASEREG',3:'ADDR',4:'CONST',5:'DEREF2',6:'DEREF',7:'ADD'}
for off in order:
    tag, A = dies[off]
    if tag is None: continue
    if 0x0050 in A: ft[A[0x0050]] += 1
    if 0x0060 in A: b = A[0x0060]; modft[struct.unpack('>H', b[-2:])[0]] += 1; mods[tuple(b[:-2])] += 1
    if 0x0080 in A: mods[tuple(A[0x0080][:-4])] += 1
    if tag == 1 and 0x00a0 in A:
        b = A[0x00a0]; i = 0
        while i < len(b):
            fmt = b[i]; i += 1; arrfmt[fmt] += 1
            if fmt == 8: at, = struct.unpack('>H', b[i:i+2]); i += 2; fm = at & 0xf; i += {2:4,5:2,6:4,3:2+struct.unpack('>H', b[i:i+2])[0] if fm==3 else 0}.get(fm, 0); break
            elif fmt == 0: i += 2 + 8
            else: break
    if 0x0020 in A and tag in (0x5, 0xc, 0x7, 0xd, 0x1c):
        ops = locdesc(A[0x0020]); locops[(hex(tag), tuple(OPN.get(o, o) for o, _ in ops))] += 1
    if tag == 4: enumsz[A.get(0x00b0)] += 1
    if tag in (0x13, 0x17, 0x2, 0x4, 0x16):
        n = A.get(0x0030)
        if n is None or n.startswith('@'): anon[(hex(tag), (n or 'NONE')[:6])] += 1
        if tag in (0x13, 0x17, 0x2) and 0x00b0 not in A: decl[hex(tag)] += 1
    if 0x00d0 in A: bit += 1
print('fund types:', {hex(k): c for k, c in ft.most_common()})
print('mod fund base:', {hex(k): c for k, c in modft.most_common()})
print('modifier seqs:', mods.most_common(10))
print('array formats:', arrfmt)
print('loc patterns:', locops.most_common(20))
print('enum sizes:', enumsz)
print('anon names:', anon.most_common(10))
print('bitfields:', bit, ' incomplete struct decls:', decl)
# name collisions: same name, different layout
lay = collections.defaultdict(set)
for off in order:
    tag, A = dies[off]
    if tag in (0x13, 0x17, 0x4) and A.get(0x0030) and 0x00b0 in A:
        lay[(tag, A[0x0030])].add(A[0x00b0])
coll = {k: v for k, v in lay.items() if len(v) > 1}
print('type names:', len(lay), ' same name w/ different sizes:', len(coll), list(coll.items())[:8])
td = collections.defaultdict(set)
for off in order:
    tag, A = dies[off]
    if tag == 0x16 and A.get(0x0030): td[A[0x0030]].add(tuple(sorted((k, v if not isinstance(v, int) or k != 0x0070 else 'ref') for k, v in A.items() if k in (0x50, 0x60, 0x80))))
print('typedef names:', len(td))
# tree depth check: count DIEs whose sibling ptr skips over children and nested closings at same offset
closings = collections.Counter()
for off in order:
    tag, A = dies[off]
    if tag is not None and 0x0010 in A: closings[A[0x0010]] += 1
print('max DIEs ending at the same offset:', max(closings.values()), ' offsets with >1:', sum(1 for v in closings.values() if v > 1))

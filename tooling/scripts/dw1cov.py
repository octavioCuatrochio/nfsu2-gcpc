import sys, struct, collections
f = open(sys.argv[1], 'rb').read()
shoff, = struct.unpack('>I', f[0x20:0x24]); shentsize, shnum, shstrndx = struct.unpack('>HHH', f[0x2e:0x34])
secs = [struct.unpack('>IIIIIIIIII', f[shoff+i*shentsize: shoff+(i+1)*shentsize]) for i in range(shnum)]
stro = secs[shstrndx][4]
def nm(o): return f[stro+o: f.index(b'\0', stro+o)].decode()
dbg = next(s for s in secs if nm(s[0]) == '.debug'); d = f[dbg[4]: dbg[4]+dbg[5]]
tags = collections.Counter(); cus = []; funcs_with_pc = 0; p = 0
while p + 4 <= len(d):
    ln, = struct.unpack('>I', d[p:p+4]); end = p + ln
    if ln < 8: p += max(ln, 4); continue
    tag, = struct.unpack('>H', d[p+4:p+6]); q = p + 6; tags[tag] += 1; name=None; lowpc=None
    while q < end:
        at, = struct.unpack('>H', d[q:q+2]); q += 2; form = at & 0xf
        if form in (1,2,6): v = struct.unpack('>I', d[q:q+4])[0]; q += 4
        elif form == 5: v = struct.unpack('>H', d[q:q+2])[0]; q += 2
        elif form == 7: v = 0; q += 8
        elif form == 3: n, = struct.unpack('>H', d[q:q+2]); v=None; q += 2+n
        elif form == 4: n, = struct.unpack('>I', d[q:q+4]); v=None; q += 4+n
        elif form == 8: e = d.index(b'\0', q); v = d[q:e].decode('latin1'); q = e+1
        if at == 0x0038: name = v
        if at == 0x0111: lowpc = v
    if tag == 0x11: cus.append(name)
    if tag in (0x6, 0x14) and lowpc: funcs_with_pc += 1
    p = end
N = {0x11:'compile_unit',0x6:'global_subroutine',0x14:'subroutine',0x7:'global_variable',0xc:'local_variable',0x5:'formal_parameter',0x13:'structure_type',0x2:'class_type',0xd:'member',0x16:'typedef',0x4:'enumeration_type',0x17:'union_type',0x15:'subroutine_type',0xb:'lexical_block',0x1:'array_type',0x1c:'inheritance'}
print({N.get(t,hex(t)): c for t, c in tags.most_common(16)})
print('functions with code address:', funcs_with_pc)
print('compile units:', len(cus))
top = collections.Counter('/'.join(c.replace('\\','/').split('/')[:6]) for c in cus)
for k, c in top.most_common(15): print(f'  {c:4} {k}')

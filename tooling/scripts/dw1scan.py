import sys, struct, collections
f = open(sys.argv[1], 'rb').read()
# ELF32 BE section headers
shoff, = struct.unpack('>I', f[0x20:0x24]); shentsize, shnum, shstrndx = struct.unpack('>HHH', f[0x2e:0x34])
secs = [struct.unpack('>IIIIIIIIII', f[shoff+i*shentsize: shoff+(i+1)*shentsize]) for i in range(shnum)]
stro = secs[shstrndx][4]
def nm(o): return f[stro+o: f.index(b'\0', stro+o)].decode()
dbg = next(s for s in secs if nm(s[0]) == '.debug'); d = f[dbg[4]: dbg[4]+dbg[5]]
TAGS = {0x11:'compile_unit',0x2e:'global_subroutine',0x2f:'global_variable',0x34:'local_variable',0x5:'formal_parameter',0x6:'global_subroutine?',0x0a:'label',0x0b:'lexical_block',0x0d:'member',0x13:'structure_type',0x2:'class_type',0x4:'enumeration_type',0x16:'typedef',0x14:'subroutine',0x15:'subroutine_type',0x17:'union_type',0x1:'array_type',0x8:'entry_point',0xf:'pointer_type',0x10:'reference_type',0x18:'unspecified_parameters',0x1d:'inlined_subroutine',0x1c:'inheritance',0x26:'const?',0x1a:'string_type'}
FORMS = {1:'ADDR',2:'REF',3:'BLOCK2',4:'BLOCK4',5:'DATA2',6:'DATA4',7:'DATA8',8:'STRING'}
stats = collections.Counter(); bytag = collections.defaultdict(collections.Counter); samples = collections.defaultdict(list)
unk_tags = collections.Counter(); p = 0; dies = 0
while p + 4 <= len(d):
    ln, = struct.unpack('>I', d[p:p+4]); end = p + ln
    if ln < 8: p = p + max(ln, 4); continue
    tag, = struct.unpack('>H', d[p+4:p+6]); q = p + 6; dies += 1; name = None; attrs = []
    if tag not in TAGS: unk_tags[tag] += 1
    while q < end:
        at, = struct.unpack('>H', d[q:q+2]); q += 2; form = at & 0xf
        if form == 1 or form == 2 or form == 6: v = struct.unpack('>I', d[q:q+4])[0]; q += 4
        elif form == 5: v = struct.unpack('>H', d[q:q+2])[0]; q += 2
        elif form == 7: v = struct.unpack('>Q', d[q:q+8])[0]; q += 8
        elif form == 3: n, = struct.unpack('>H', d[q:q+2]); v = d[q+2:q+2+n]; q += 2+n
        elif form == 4: n, = struct.unpack('>I', d[q:q+4]); v = d[q+4:q+4+n]; q += 4+n
        elif form == 8: e = d.index(b'\0', q); v = d[q:e].decode('latin1'); q = e+1
        else: raise SystemExit(f'bad form {at:#x} at {p:#x}')
        if (at & 0xfff0) == 0x0030: name = v
        attrs.append((at, v))
    for at, v in attrs:
        if (at & 0xfff0) >= 0x2000:
            stats[at] += 1; bytag[at][TAGS.get(tag, hex(tag))] += 1
            if len(samples[at]) < 6: samples[at].append((hex(p), TAGS.get(tag, hex(tag)), name, v.hex() if isinstance(v, bytes) else v))
    p = end
print('DIEs:', dies, ' unknown tags:', {hex(k): c for k, c in unk_tags.most_common(10)})
for at, c in stats.most_common():
    print(f'\nattr {at:#06x} name={at & 0xfff0:#06x} form={FORMS.get(at & 0xf)} count={c} tags={dict(bytag[at])}')
    for s in samples[at]: print('   ', s)

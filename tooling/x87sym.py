#!/usr/bin/env python3
"""Symbolic executor for straight-line x87 code (no branches): prints C++ that evaluates the same operations in the
same order, as `long double` (x87 registers), rounding to float exactly where the original stores DWORDs.

usage: x87sym.py <exe> <start> <stop> arg0=name arg1=name ...   (cdecl args: arg0 = [esp+4] at entry)
Memory operands are tracked symbolically: [esp+N] stack slots and [reg+N] through registers loaded from args.
Output: one statement per DWORD store, e.g. `out[3] = float(...)`, temporaries for stack slots.
Unsupported instructions abort with a message (extend as needed)."""
import re, subprocess, sys

exe, start, stop = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3], 16)
argnames = dict(a.split('=') for a in sys.argv[4:])

asm = subprocess.run(['objdump', '-d', '-M', 'intel', '--no-show-raw-insn', f'--start-address={start}',
                      f'--stop-address={stop}', exe], capture_output=True, text=True).stdout
lines = []
for l in asm.splitlines():
    m = re.match(r'\s*([0-9a-f]+):\s+(.*)$', l)
    if m and m.group(2).strip() not in ('nop', 'int3') and not m.group(2).startswith('lea    esp,[esp'):
        lines.append((int(m.group(1), 16), re.sub(r'\s+', ' ', m.group(2).strip())))

esp = 0                       # offset relative to entry esp (entry: [esp] = return address)
regs = {}                     # reg -> ('ptr', argname) | ('val', expr) | ('int', int)
mem = {}                      # (base, off) -> expr string (symbolic float value)
st = []                       # x87 stack, st[0] = top
out = []
tmpn = [0]


def addr(op):
    m = re.match(r'DWORD PTR \[(\w+)(?:([+-])(0x[0-9a-f]+|\d+))?\]', op)
    if not m:
        raise SystemExit(f'bad mem operand {op}')
    reg, sign, off = m.group(1), m.group(2), m.group(3)
    off = int(off, 0) if off else 0
    if sign == '-':
        off = -off
    if reg == 'esp':
        pos = esp + off
        if pos > 0 and pos % 4 == 0 and f'arg{pos // 4 - 1}' in argnames and ('stack', pos) not in mem:
            return ('arg', argnames[f'arg{pos // 4 - 1}'])
        return ('stack', pos)
    r = regs.get(reg)
    if r and r[0] == 'ptr':
        return (r[1], off)
    raise SystemExit(f'unknown base {reg} in {op}')


def const(va):
    """float constant at an absolute VA, as an exact C++ hex-float literal"""
    import struct
    with open(exe, 'rb') as f:
        d = f.read(0x400)
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        ns = struct.unpack_from('<H', d, pe + 6)[0]
        opt = struct.unpack_from('<H', d, pe + 20)[0]
        for i in range(ns):
            o = pe + 24 + opt + i * 40
            vs, va0, rs, ro = struct.unpack_from('<IIII', d, o + 8)
            if 0x400000 + va0 <= va < 0x400000 + va0 + vs:
                f.seek(ro + va - 0x400000 - va0)
                v = struct.unpack('<f', f.read(4))[0]
                return f'L({float(v).hex()}f)'
    raise SystemExit(f'constant {va:x} not in a section')


def load(op):
    m = re.match(r'DWORD PTR ds:(0x[0-9a-f]+)$', op)
    if m:
        return const(int(m.group(1), 16))
    a = addr(op)
    if a in mem:
        return mem[a]
    if a[0] == 'arg':
        return f'L({a[1]})'
    if a[0] == 'stack':
        raise SystemExit(f'read of unwritten stack slot {a}')
    return f'L({a[0]}[{a[1] // 4}])'


def store(op, val, rounded=True):
    a = addr(op)
    if a[0] in ('stack', 'arg'):
        tmpn[0] += 1
        name = f't{tmpn[0]}'
        out.append(f'const float {name} = float({val});')
        mem[a] = f'L({name})'
    else:
        out.append(f'{a[0]}[{a[1] // 4}] = float({val});')
        mem[a] = f'L({a[0]}[{a[1] // 4}])'


def sti(s):
    m = re.match(r'st(?:\((\d)\))?$', s)
    return int(m.group(1) or 0)


for va, ins in lines:
    op, _, rest = ins.partition(' ')
    ops = [o.strip() for o in rest.split(',')] if rest else []
    if op == 'ret':
        break
    if op == 'lea':
        out.append(f'// {va:x}: {ins}'); continue
    if op == 'call':
        out.append(f'// {va:x}: {ins} (stop)'); break
    if op == 'push':
        esp -= 4; continue
    if op == 'pop':
        esp += 4; continue
    if op == 'sub' and ops[0] == 'esp':
        esp -= int(ops[1], 0); continue
    if op == 'add' and ops[0] == 'esp':
        esp += int(ops[1], 0); continue
    if op == 'and' and ops[0] == 'esp':
        continue                                    # alignment: stack slots stay symbolic
    if op == 'mov' and ops[0] == 'ebp' and ops[1] == 'esp':
        regs['ebp_esp'] = esp; continue
    if op == 'mov' and ops[0] in ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi'):
        if ops[1].startswith('DWORD PTR [ebp'):
            off = int(re.search(r'\+(0x[0-9a-f]+|\d+)', ops[1]).group(1), 0)
            pos = regs['ebp_esp'] + off             # [ebp+8] = arg0 after push ebp; mov ebp,esp
            name = argnames.get(f'arg{(pos // 4) - 1}')
            regs[ops[0]] = ('ptr', name); continue
        if ops[1].startswith('DWORD PTR'):
            a = addr(ops[1])
            if a[0] == 'arg':
                regs[ops[0]] = ('ptr', a[1]); continue
            regs[ops[0]] = ('val', load(ops[1])); continue
        if ops[1].startswith('0x') or ops[1].isdigit():
            regs[ops[0]] = ('int', int(ops[1], 0)); continue
        if ops[1] in regs:
            regs[ops[0]] = regs[ops[1]]; continue
        raise SystemExit(f'{va:x}: {ins}')
    if op == 'xor' and ops[0] == ops[1]:
        regs[ops[0]] = ('int', 0); continue
    if op == 'mov' and ops[0].startswith('DWORD PTR'):
        r = regs.get(ops[1]) if not ops[1].startswith('0x') else ('int', int(ops[1], 0))
        if r is None:
            raise SystemExit(f'{va:x}: {ins}')
        a = addr(ops[0])
        if r[0] == 'val':
            if a[0] in ('stack', 'arg'):
                mem[a] = r[1]
            else:
                out.append(f'std::memcpy(&{a[0]}[{a[1] // 4}], &/*bits of*/{r[1]}, 4); // raw copy')
                mem[a] = f'L({a[0]}[{a[1] // 4}])'
        elif r[0] == 'int':
            if a[0] in ('stack', 'arg'):
                mem[a] = f'L(bitsf(0x{r[1]:08X}u))'
            else:
                out.append(f'{a[0]}[{a[1] // 4}] = bitsf(0x{r[1]:08X}u);')
                mem[a] = f'L({a[0]}[{a[1] // 4}])'
        continue
    # x87
    if op == 'fld':
        st.insert(0, st[sti(ops[0])] if ops[0].startswith('st') else load(ops[0])); continue
    if op in ('fst', 'fstp'):
        if ops[0].startswith('st'):
            st[sti(ops[0])] = st[0]
        else:
            v = st[0]
            store(ops[0], v)
            if op == 'fst':
                a = addr(ops[0])
                # value stays in the register unrounded; memory now holds the float
        if op == 'fstp':
            st.pop(0)
        continue
    if op == 'fxch':
        i = sti(ops[0]) if ops else 1
        st[0], st[i] = st[i], st[0]; continue
    if op == 'fchs':
        st[0] = f'Fchs({st[0]})'; continue   # exact x87 fchs (sign flip after the operation, NaN too)
    if op == 'fabs':
        st[0] = f'__builtin_fabsl({st[0]})'; continue
    if op == 'fsqrt':
        st[0] = f'__builtin_sqrtl({st[0]})'; continue
    m = re.match(r'f(add|sub|subr|mul|div|divr)(p?)$', op)
    if m:
        kind, pop = m.group(1), m.group(2)
        sym = {'add': '+', 'sub': '-', 'subr': '-', 'mul': '*', 'div': '/', 'divr': '/'}[kind]
        if len(ops) == 1 and not ops[0].startswith('st'):   # st0 = st0 op mem
            a, b = st[0], load(ops[0])
            st[0] = f'({b} {sym} {a})' if kind in ('subr', 'divr') else f'({a} {sym} {b})'
            continue
        if not ops:
            ops = ['st(1)', 'st']
        d, s = sti(ops[0]), sti(ops[1])
        a, b = st[d], st[s]
        st[d] = f'({b} {sym} {a})' if kind in ('subr', 'divr') else f'({a} {sym} {b})'
        if pop:
            st.pop(0)
        continue
    raise SystemExit(f'unsupported {va:x}: {ins}')

print('\n'.join(out))
if st:
    print(f'// x87 stack left: {st}')

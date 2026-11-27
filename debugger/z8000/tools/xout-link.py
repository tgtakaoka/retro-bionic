#!/usr/bin/env python3
"""Link CP/M-8000 x.out objects into an Intel HEX image.

    debugger/z8000/tools/xout-link.py --org ADDR [--symbols LISTING...]
        [--define NAME=VALUE...] [--provide NAME...] [--patch HEX...]
        [--keep NAME...] [--overlay HEX...] [--map OUT.inc] [--export NAME...]
        [-o OUT.hex] OBJECT... [ARCHIVE...]

The objects are DRI's relocatable x.out files (xout.h of the P-CP/M-Z8K
sources), such as CP/M-8000 1.1's CPMSYS.REL, and the archives ar8k's,
such as LIBCPM.A, searched for the members that define what is still
undefined. Every code segment goes first, from --org, then the constant,
data and bss segments, each on a word boundary, in the order the files
name them, then the commons, the uninitialised C variables an object
leaves undefined with their size as the value. The bss and commons come
out as zeros, so `L` clears them.

The BIOS defines what the kernel needs from it: --symbols takes the
labels of a libasm listing, --define one value, and --provide names one
the BIOS will define, so no library member is pulled for it; --org may
name one of them. --patch writes a HEX file's bytes over the kernel's, which is how
the Z8002 glue replaces its routines, and --keep names a symbol a patch
must not reach; --overlay adds a HEX file, the BIOS, where the kernel
has nothing. --map writes the first object's symbols, its commons too, as EQUs
for the BIOS to include, and each --export NAME, a global of another
object; with --map alone, undefined symbols are allowed.

Only non-segmented objects link: a segmented relocation stops the link.
"""
import re
import struct
import sys

MAGIC_NONSEG = (0xEE02, 0xEE03, 0xEE06, 0xEE07, 0xEE0A, 0xEE0B)
SG_BSS, SG_STK, SG_COD, SG_CON, SG_DAT = 1, 2, 3, 4, 5
RL_OFF, RL_XOF = 1, 5
SY_LOC, SY_UNX, SY_GLB = 1, 2, 3
AR_MAGIC = b'\xff\x65'
# The order the segment types are laid out in.
ORDER = [SG_COD, 6, 7, SG_CON, SG_DAT, SG_BSS, SG_STK]


class Xout:
    """One object: segments, code, relocations and symbols."""

    def __init__(self, name, data):
        self.name = name
        magic, nseg, init, reloc, symb = struct.unpack('>HhIII', data[:16])
        if magic not in MAGIC_NONSEG:
            sys.exit(f'{name}: not a non-segmented x.out (magic {magic:04X})')
        self.segs = [struct.unpack('>BBH', data[16 + 4 * i:20 + 4 * i])
                     for i in range(nseg)]
        pos = 16 + 4 * nseg
        self.code = bytearray(data[pos:pos + init])
        pos += init
        self.relocs = [struct.unpack('>BBHH', data[pos + 6 * i:pos + 6 * i + 6])
                       for i in range(reloc // 6)]
        pos += reloc
        self.syms = []
        for i in range(symb // 12):
            sg, fl, val, sym = struct.unpack('>BBH8s', data[pos + 12 * i:pos + 12 * i + 12])
            self.syms.append((sg, fl, val, sym.rstrip(b'\0').decode('ascii')))
        self.base = [None] * nseg  # each segment's address
        self.offset = []  # each segment's offset in the code part
        at = 0
        for _, typ, length in self.segs:
            self.offset.append(at)
            if typ not in (SG_BSS, SG_STK):
                at += length

    def defines(self):
        return {s[3] for s in self.syms if s[1] == SY_GLB}

    def undefined(self):
        return {s[3] for s in self.syms if s[1] == SY_UNX}

    def commons(self):
        """The (name, size) of the undefined symbols with a size."""
        return [(s[3], s[2]) for s in self.syms if s[1] == SY_UNX and s[2]]


def archive(name, data):
    """The (member name, bytes) of an ar8k archive."""
    if data[:2] != AR_MAGIC:
        sys.exit(f'{name}: not an ar8k archive')
    pos = 2
    while pos + 26 <= len(data):
        member, _, _, _, _, size = struct.unpack('>14sIBBHI', data[pos:pos + 26])
        member = member.rstrip(b'\0').decode('ascii')
        pos += 26
        if not member or size == 0:
            break
        yield f'{name}({member})', data[pos:pos + size]
        pos += size


def read_listing(path, symbols):
    """The labels of a libasm listing: each line's address, or the EQU
    value after '='."""
    line_re = re.compile(
        r'\s*(?:\(\d+\))?\s*([0-9A-F]{1,8}) :\s+(?:=([0-9A-F]+)\s+)?'
        r'(?:[0-9A-F]{2,4}\s)*\s*([A-Za-z_.$][\w.$]*):')
    with open(path) as f:
        for line in f:
            m = line_re.match(line)
            if m:
                symbols[m.group(3)] = int(m.group(2) or m.group(1), 16)


def read_hex(path):
    """An Intel HEX file's bytes as {address: byte}."""
    image = {}
    high = 0
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line.startswith(':'):
                continue
            rec = bytes.fromhex(line[1:])
            count, addr, kind = rec[0], (rec[1] << 8) | rec[2], rec[3]
            if sum(rec) & 0xFF:
                sys.exit(f'{path}: checksum error')
            data = rec[4:4 + count]
            if kind == 0:
                for i, b in enumerate(data):
                    image[high + addr + i] = b
            elif kind == 4:
                high = ((data[0] << 8) | data[1]) << 16
            elif kind == 2:
                high = ((data[0] << 8) | data[1]) << 4
            elif kind == 1:
                break
    return image


def ihex(kind, addr, data):
    rec = bytes([len(data), addr >> 8, addr & 0xFF, kind]) + data
    return ':%s%02X\n' % (rec.hex().upper(), (-sum(rec)) & 0xFF)


def write_hex(path, image):
    with open(path, 'w') as f:
        high = None
        addrs = sorted(image)
        i = 0
        while i < len(addrs):
            start = addrs[i]
            run = bytearray()
            while i < len(addrs) and addrs[i] == start + len(run) and \
                    len(run) < 32 and (addrs[i] >> 16) == (start >> 16):
                run.append(image[addrs[i]])
                i += 1
            if start >> 16 != high:
                high = start >> 16
                f.write(ihex(4, 0, bytes([high >> 8, high & 0xFF])))
            f.write(ihex(0, start & 0xFFFF, bytes(run)))
        f.write(ihex(1, 0, b''))


def main(argv):
    org = None
    out = mapfile = None
    symbols = {}
    patches = []
    keeps = []
    provides = set()
    exports = set()
    overlays = []
    files = []
    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg in ('--org', '-o', '--map', '--symbols', '--define', '--provide',
                   '--patch', '--keep', '--overlay', '--export'):
            i += 1
            if i >= len(argv):
                sys.exit(__doc__)
            val = argv[i]
            if arg == '--org':
                org = val
            elif arg == '-o':
                out = val
            elif arg == '--map':
                mapfile = val
            elif arg == '--symbols':
                read_listing(val, symbols)
            elif arg == '--define':
                name, _, value = val.partition('=')
                symbols[name] = int(value, 0)
            elif arg == '--provide':
                provides.add(val)
            elif arg == '--patch':
                patches.append(val)
            elif arg == '--keep':
                keeps.append(val)
            elif arg == '--export':
                exports.add(val)
            else:
                overlays.append(val)
        elif arg.startswith('-'):
            sys.exit(__doc__)
        else:
            files.append(arg)
        i += 1
    if org is None or not files or (out is None and mapfile is None):
        sys.exit(__doc__)
    if org in symbols:
        org = symbols[org]
    else:
        try:
            org = int(org, 0)
        except ValueError:
            sys.exit(f'--org {org}: not an address or a symbol')
    provided = set(symbols)

    objects = []
    members = []
    for path in files:
        with open(path, 'rb') as f:
            data = f.read()
        if data[:2] == AR_MAGIC:
            members.extend(archive(path, data))
        else:
            objects.append(Xout(path, data))
    # Pull archive members until nothing they define is still needed.
    while True:
        defined = set(symbols) | provides
        for obj in objects:
            defined |= obj.defines()
        needed = set()
        for obj in objects:
            needed |= obj.undefined() - defined
        pulled = False
        for i, (name, data) in enumerate(members):
            member = Xout(name, data)
            if member.defines() & needed:
                objects.append(member)
                del members[i]
                pulled = True
                break
        if not pulled:
            break

    # Lay the segments out by type, in file order within a type.
    at = org
    for typ in ORDER:
        for obj in objects:
            for n, (_, t, length) in enumerate(obj.segs):
                if t == typ:
                    obj.base[n] = at
                    at += (length + 1) & ~1
    for obj in objects:
        for n, (_, t, length) in enumerate(obj.segs):
            if obj.base[n] is None:
                sys.exit(f'{obj.name}: segment {n} has unknown type {t}')

    # Every symbol's value: the BIOS's, then the globals, then the
    # commons, which take the room after the bss, then the first object's
    # locals for the map.
    globals_ = set()
    for obj in objects:
        for sg, fl, val, name in obj.syms:
            if fl == SY_GLB:
                if name in symbols and symbols[name] != obj.base[sg] + val:
                    sys.exit(f'{obj.name}: {name} defined twice')
                symbols[name] = obj.base[sg] + val
                globals_.add(name)
    # A common a listing already names must land where it says.
    commons = []
    exported = objects[0].defines()
    for obj in objects:
        for name, size in obj.commons():
            if name in globals_ or name in provides or (name, size) in commons:
                continue
            if symbols.get(name, at) != at:
                sys.exit(f'{obj.name}: common {name} moved from {symbols[name]:04X}H')
            symbols[name] = at
            commons.append((at, size))
            exported.add(name)
            at += (size + 1) & ~1
    end = at
    locals_ = {}
    for sg, fl, val, name in objects[0].syms:
        if fl == SY_LOC and sg < len(objects[0].segs):
            locals_[name] = None if name in locals_ else objects[0].base[sg] + val
    undefined = set()
    for obj in objects:
        undefined |= obj.undefined() - set(symbols) - provides
    if undefined and out is not None:
        sys.exit('undefined: ' + ' '.join(sorted(undefined)))

    # Relocate: a 16-bit word gets its segment's base, or its external's
    # value, added.
    image = {}
    for obj in objects:
        for sg, typ, loc, bas in obj.relocs:
            if typ == 0:
                continue
            if typ not in (RL_OFF, RL_XOF):
                sys.exit(f'{obj.name}: segmented relocation type {typ}')
            if typ == RL_OFF:
                add = obj.base[bas]
            else:
                _, fl, _, name = obj.syms[bas]
                add = symbols.get(name, 0)
            pos = obj.offset[sg] + loc
            word = (struct.unpack('>H', obj.code[pos:pos + 2])[0] + add) & 0xFFFF
            obj.code[pos:pos + 2] = struct.pack('>H', word)
        for n, (_, t, length) in enumerate(obj.segs):
            if t in (SG_BSS, SG_STK):
                data = bytes(length)
            else:
                data = obj.code[obj.offset[n]:obj.offset[n] + length]
            for k, b in enumerate(data):
                image[obj.base[n] + k] = b
    for base, size in commons:
        for k in range(size):
            image[base + k] = 0
    kept = {symbols[name] for name in keeps if name in symbols}
    for path in patches:
        patch = read_hex(path)
        outside = [a for a in patch if a not in image]
        if outside:
            sys.exit(f'{path}: patches outside the kernel at {min(outside):04X}H')
        if kept & set(patch):
            sys.exit(f'{path}: reaches a kept symbol at {min(kept & set(patch)):04X}H')
        image.update(patch)
    for path in overlays:
        overlay = read_hex(path)
        inside = set(overlay) & set(image)
        if inside:
            sys.exit(f'{path}: overlaps the kernel at {min(inside):04X}H')
        image.update(overlay)

    if mapfile:
        with open(mapfile, 'w') as f:
            f.write(f';;; Made by xout-link.py from {" ".join(files)}: the\n'
                    f';;; symbols of the system linked at {org:04X}H,'
                    f' ending at {end:04X}H.\n')
            for name, value in sorted(symbols.items(), key=lambda kv: kv[1]):
                if (name in exported or name in exports) and name not in provided:
                    f.write(f'{name}:\tequ\t0{value:04X}H\n')
            f.write(';;; The kernel\'s locals.\n')
            for name, value in sorted(locals_.items(), key=lambda kv: kv[1] or 0):
                if value is not None:
                    f.write(f'{name}:\tequ\t0{value:04X}H\n')
    if out:
        write_hex(out, image)


if __name__ == '__main__':
    main(sys.argv[1:])

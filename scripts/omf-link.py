#!/usr/bin/env python3
"""Link Intel OMF objects after a flat binary into one memory image.

    scripts/omf-link.py inc OBJ... > NAME.inc
    scripts/omf-link.py link -o OUT.SYS --base SEG --bin BIOS.hex \\
        [--define NAME=OFFSET...] [--check NAME.inc] OBJ...

For MS-DOS 2.0's IO.SYS: samples/i8086/msdosbio.asm is assembled flat
and Microsoft's SYSINIT.OBJ and SYSIMES.OBJ follow it, so no MASM or
LINK is needed. `inc` prints each object's public symbols as EQUs, the
offsets in their segment, for the BIOS to include. `link` lays out the
hex file's bytes from offset 0 of the segment at paragraph SEG, then
every object's segments, which must all combine into one, at the next
paragraph, and resolves the objects' fixups. --define gives an external
the objects need from the BIOS, an offset in the hex file's segment.
--check fails the link if a symbol of an `inc` file the BIOS was
assembled with is public in the objects at another offset.

Only what a MASM 1.x object holds is handled: public segments, LEDATA
and LIDATA, and 16-bit offset, segment and far-pointer fixups to
segments and externals; anything else stops the link.
"""
import sys

THEADR, COMENT, MODEND, EXTDEF, PUBDEF, LNAMES = 0x80, 0x88, 0x8A, 0x8C, 0x90, 0x96
SEGDEF, GRPDEF, FIXUPP, LEDATA, LIDATA = 0x98, 0x9A, 0x9C, 0xA0, 0xA2


def read_hex(path):
    """The Intel HEX file's bytes as a dict of address: byte."""
    mem = {}
    base = 0
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line.startswith(':'):
                continue
            rec = bytes.fromhex(line[1:])
            count, addr, kind = rec[0], (rec[1] << 8) | rec[2], rec[3]
            data = rec[4:4 + count]
            if kind == 0:
                for i, b in enumerate(data):
                    mem[base + addr + i] = b
            elif kind == 4:
                base = ((data[0] << 8) | data[1]) << 16
            elif kind == 2:
                base = ((data[0] << 8) | data[1]) << 4
    return mem


def records(data):
    """(type, body) of each record up to MODEND."""
    i = 0
    while i + 3 <= len(data):
        kind = data[i]
        length = data[i + 1] | (data[i + 2] << 8)
        yield kind, data[i + 3:i + 2 + length]  # without the checksum
        if kind in (MODEND, MODEND + 1):
            return
        i += 3 + length


def index(body, i):
    """An OMF index at body[i]: (value, next i)."""
    if body[i] & 0x80:
        return ((body[i] & 0x7F) << 8) | body[i + 1], i + 2
    return body[i], i + 1


def name(body, i):
    n = body[i]
    return body[i + 1:i + 1 + n].decode('ascii'), i + 1 + n


class Module:
    """One object's segments, symbols, data and fixups."""

    def __init__(self, path):
        self.path = path
        self.names = ['']
        self.segments = [None]  # (name, length, align code) by index
        self.externs = [None]
        self.publics = []  # (name, segment index, offset)
        self.data = []  # (segment index, offset, bytes)
        # For LIDATA records, by index in data: the offset in the expanded
        # bytes of each data byte's offset in the record, a fixup's unit.
        self.iterated = {}
        # (data index, offset in data, loc, mode, frame, target, displacement)
        self.fixups = []
        with open(path, 'rb') as f:
            raw = f.read()
        for kind, body in records(raw):
            self.record(kind, body)

    def fail(self, what):
        sys.exit(f'{self.path}: {what}')

    def record(self, kind, body):
        if kind == LNAMES:
            i = 0
            while i < len(body):
                n, i = name(body, i)
                self.names.append(n)
        elif kind == SEGDEF:
            acbp = body[0]
            align, combine = acbp >> 5, (acbp >> 2) & 7
            i = 1
            if align == 0:
                self.fail('absolute segment')
            length = body[i] | (body[i + 1] << 8)
            i += 2
            if acbp & 2:
                length = 0x10000
            seg, i = index(body, i)
            if align not in (1, 2, 3) or combine != 2:
                self.fail(f'segment {self.names[seg]} not public, or aligned '
                          'past a paragraph')
            self.segments.append((self.names[seg], length, align))
        elif kind == GRPDEF:
            self.fail('group')
        elif kind == EXTDEF:
            i = 0
            while i < len(body):
                n, i = name(body, i)
                _, i = index(body, i)  # type
                self.externs.append(n)
        elif kind == PUBDEF:
            group, i = index(body, 0)
            seg, i = index(body, i)
            if seg == 0:
                self.fail('absolute public')
            while i < len(body):
                n, i = name(body, i)
                offset = body[i] | (body[i + 1] << 8)
                _, i = index(body, i + 2)  # type
                self.publics.append((n, seg, offset))
        elif kind == LEDATA:
            seg, i = index(body, 0)
            offset = body[i] | (body[i + 1] << 8)
            self.data.append((seg, offset, bytearray(body[i + 2:])))
        elif kind == LIDATA:
            seg, i = index(body, 0)
            offset = body[i] | (body[i + 1] << 8)
            out = bytearray()
            start = at = i + 2
            places = {}
            while at < len(body):
                at = self.expand(body, at, out, start, places)
            self.data.append((seg, offset, out))
            self.iterated[len(self.data) - 1] = places
        elif kind == FIXUPP:
            self.fixupp(body)
        elif kind in (THEADR, COMENT, MODEND):
            pass
        else:
            self.fail(f'record type {kind:02X}')

    def expand(self, body, i, out, start, places):
        """Expands the iterated data block at body[i] into out, noting in
        places where each data byte of a block repeated once landed, as
        an offset from start."""
        repeat = body[i] | (body[i + 1] << 8)
        blocks = body[i + 2] | (body[i + 3] << 8)
        i += 4
        if blocks == 0:
            n = body[i]
            chunk = body[i + 1:i + 1 + n]
            if repeat == 1:
                for k in range(n):
                    places[i + 1 + k - start] = len(out) + k
            i += 1 + n
            out += chunk * repeat
            return i
        inner = bytearray()
        inner_places = {}
        for _ in range(blocks):
            i = self.expand(body, i, inner, start, inner_places)
        if repeat == 1:
            for raw, at in inner_places.items():
                places[raw] = len(out) + at
        out += inner * repeat
        return i

    def fixupp(self, body):
        threads = {}  # (is frame, number): (method, index)
        i = 0
        while i < len(body):
            b = body[i]
            if b & 0x80 == 0:  # thread
                is_frame, method, number = b & 0x40, (b >> 2) & 7, b & 3
                i += 1
                datum = 0
                if is_frame and method < 3 or not is_frame and method < 4:
                    datum, i = index(body, i)
                threads[(bool(is_frame), number)] = (method, datum)
                continue
            mode = 'seg' if b & 0x40 else 'self'
            loc = (b >> 2) & 0xF
            offset = ((b & 3) << 8) | body[i + 1]
            fixdat = body[i + 2]
            i += 3
            if fixdat & 0x80:
                frame = threads[(True, (fixdat >> 4) & 3)]
            else:
                method = (fixdat >> 4) & 7
                datum = 0
                if method < 3:
                    datum, i = index(body, i)
                frame = (method, datum)
            if fixdat & 0x08:
                target = threads[(False, fixdat & 3)]
            else:
                datum, i = index(body, i)
                target = (fixdat & 3, datum)
            disp = 0
            if not fixdat & 0x04:
                disp = body[i] | (body[i + 1] << 8)
                i += 2
            d = len(self.data) - 1
            if d in self.iterated:
                if offset not in self.iterated[d]:
                    self.fail('fixup on repeated data')
                offset = self.iterated[d][offset]
            self.fixups.append((d, offset, loc, mode, frame, target, disp))


class Link:
    """The objects' segments after the flat binary at paragraph |base|."""

    def __init__(self, base, bios, modules, defines):
        self.base = base
        self.bios = bios  # bytes of the flat binary
        self.modules = modules
        self.defines = defines  # name: offset in the binary's segment
        paras = (len(bios) + 15) // 16
        self.segment = base + paras  # the one combined segment
        self.image = bytearray(bios) + bytes(paras * 16 - len(bios))
        self.place = {}  # (module, segment index): offset in the segment
        self.length = 0
        self.publics = {}  # name: offset in the segment
        self.name = None
        for m in modules:
            for s, seg in enumerate(m.segments):
                if seg is None or seg[1] == 0:
                    continue
                if self.name is None:
                    self.name = seg[0]
                elif seg[0] != self.name:
                    m.fail(f'segment {seg[0]} does not combine with {self.name}')
                mask = {1: 0, 2: 1, 3: 15}[seg[2]]
                self.length = (self.length + mask) & ~mask
                self.place[(m, s)] = self.length
                self.length += seg[1]
            for n, s, offset in m.publics:
                self.publics[n] = self.place[(m, s)] + offset
        if self.length > 0x10000:
            sys.exit('segment too long')
        self.image += bytes(self.length)
        for m in modules:
            self.load(m)

    def at(self, module, seg, offset=0):
        """The image offset of |offset| in |seg| of |module|."""
        return (self.segment - self.base) * 16 + self.place[(module, seg)] + offset

    def resolve(self, module, kind, datum):
        """(paragraph, offset) of a fixup's frame or target datum."""
        if kind == 0:  # segment
            return self.segment, self.place[(module, datum)]
        if kind == 2:  # external
            n = module.externs[datum]
            if n in self.publics:
                return self.segment, self.publics[n]
            if n in self.defines:
                return self.base, self.defines[n]
            module.fail(f'undefined {n}')
        module.fail(f'fixup method {kind}')

    def load(self, m):
        for seg, offset, data in m.data:
            at = self.at(m, seg, offset)
            self.image[at:at + len(data)] = data
        for d, offset, loc, mode, frame, target, disp in m.fixups:
            seg, data_offset, _ = m.data[d]
            at = self.at(m, seg, data_offset + offset)
            tseg, toff = self.resolve(m, target[0] & 3, target[1])
            toff += disp
            if frame[0] == 5:  # the target's segment
                fseg = tseg
            elif frame[0] == 4:  # the location's segment
                fseg = self.segment
            else:
                fseg, _ = self.resolve(m, frame[0], frame[1])
            if mode == 'self':
                if loc != 1:
                    m.fail(f'self-relative fixup of kind {loc}')
                here = self.segment * 16 + self.place[(m, seg)] + data_offset + offset
                value = (tseg * 16 + toff - (here + 2)) & 0xFFFF
                self.put16(at, value)
            elif loc == 1 or loc == 5:  # offset
                self.put16(at, (tseg * 16 + toff - fseg * 16) & 0xFFFF)
            elif loc == 2:  # segment
                self.put16(at, fseg)
            elif loc == 3:  # far pointer
                self.put16(at, (tseg * 16 + toff - fseg * 16) & 0xFFFF)
                self.put16(at + 2, fseg)
            else:
                m.fail(f'fixup of kind {loc}')

    def put16(self, at, value):
        value += self.image[at] | (self.image[at + 1] << 8)
        self.image[at] = value & 0xFF
        self.image[at + 1] = (value >> 8) & 0xFF


def check(incs, modules):
    """Each `inc` file's symbols the objects make public, at the same
    offsets in their segments."""
    publics = {n: offset for m in modules for n, _, offset in m.publics}
    for inc in incs:
        with open(inc) as f:
            for line in f:
                n, colon, rest = line.partition(':')
                if not colon or line.startswith(';'):
                    continue
                offset = int(rest.split()[1].rstrip('Hh'), 16)
                if n in publics and publics[n] != offset:
                    sys.exit(f'{inc}: {n} is {offset:04X}H, '
                             f'the objects have {publics[n]:04X}H')


def main(argv):
    if len(argv) < 2:
        sys.exit(__doc__)
    command, argv = argv[0], argv[1:]
    if command == 'inc':
        for m in (Module(p) for p in argv):
            print(f';;; {m.path.rsplit("/", 1)[-1]}')
            for n, seg, offset in sorted(m.publics, key=lambda p: p[2]):
                print(f'{n}:'.ljust(24) + f'equ     {offset:04X}H')
        return
    if command != 'link':
        sys.exit(__doc__)
    out = base = bios = None
    defines = {}
    checks = []
    objs = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == '-o':
            out = argv[i + 1]
            i += 2
        elif a == '--base':
            base = int(argv[i + 1], 0)
            i += 2
        elif a == '--bin':
            bios = argv[i + 1]
            i += 2
        elif a == '--check':
            checks.append(argv[i + 1])
            i += 2
        elif a == '--define':
            n, v = argv[i + 1].split('=')
            defines[n] = int(v, 0)
            i += 2
        else:
            objs.append(a)
            i += 1
    if out is None or base is None or bios is None or not objs:
        sys.exit(__doc__)
    mem = read_hex(bios)
    if not mem:
        sys.exit(f'{bios}: empty')
    first = min(mem)
    if first != 0:
        sys.exit(f'{bios}: starts at {first:X}, not 0')
    flat = bytes(mem.get(a, 0) for a in range(max(mem) + 1))
    modules = [Module(p) for p in objs]
    check(checks, modules)
    link = Link(base, flat, modules, defines)
    with open(out, 'wb') as f:
        f.write(link.image)
    print(f'{out}: {len(flat)} bytes of {bios}, {link.name} at '
          f'{link.segment:04X}:0000, {link.length} bytes')


if __name__ == '__main__':
    main(sys.argv[1:])

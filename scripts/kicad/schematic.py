"""Write a one-sheet Bionic board schematic.

Library symbols are copied from a template schematic that already has
them (by default the MC68HC16Z board), the CPU symbol comes from
symbol.CpuSymbol. Coordinates are in mm on an A4 sheet; keep them on the
1.27 mm grid so wires meet pins.
"""
import os
import uuid

from sexp import extract_block, pins_of, format_text

G = 2.54
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
TEMPLATE = os.path.join(REPO, 'schematics/mc68hc16z/bionic-mc68hc16z.kicad_sch')
STOCK = ['Device:C_Small', 'Device:R_Small', 'power:GND', 'power:VCC', 'power:PWR_FLAG',
         'bionic:Bionic-P135', 'bionic:Bionic-P245', 'microchip:11AA010-I_TT', 'rhom:CSL1901UW1']
C0603 = 'Capacitor_SMD:C_0603_1608Metric_Pad1.08x0.95mm_HandSolder'
R0603 = 'Resistor_SMD:R_0603_1608Metric_Pad0.98x0.95mm_HandSolder'


def r2(v):
    return round(v * 100) / 100


def xform(px, py, rot):
    """symbol (y up) to sheet (y down) offset, rotated counter-clockwise by rot"""
    ox, oy = px, -py
    r = int(rot) % 360
    if r == 0:
        return ox, oy
    if r == 90:
        return oy, -ox
    if r == 180:
        return -ox, -oy
    return -oy, ox


class Schematic:
    def __init__(self, project, template=TEMPLATE, symbols=STOCK):
        self.project = project
        self.root = str(uuid.uuid5(uuid.NAMESPACE_URL, project))
        self._n = 0
        self.items = []
        self.pinpos = {}
        self.libs = {}
        self.pins = {}
        self.counters = {}
        self.used = set()
        with open(template) as f:
            text = f.read()
        for n in symbols:
            self.add_library_symbol(n, extract_block(text, '(symbol "%s"' % n))

    def add_library_symbol(self, lib_id, text, pins=None):
        self.libs[lib_id] = text
        self.pins[lib_id] = pins or pins_of(text)

    def add_cpu(self, cpu, prefix='cpu:'):
        lib_id = prefix + cpu.name
        self.add_library_symbol(lib_id, cpu.text(prefix), cpu.pin_table())
        return lib_id

    def uid(self):
        self._n += 1
        return str(uuid.uuid5(uuid.NAMESPACE_URL, '%s/%d' % (self.project, self._n)))

    def next_ref(self, prefix):
        """lowest free reference with this prefix"""
        n = 1
        while '%s%d' % (prefix, n) in self.used:
            n += 1
        return '%s%d' % (prefix, n)

    @staticmethod
    def prop(name, val, x, y, hide=False, angle=0, justify=None):
        j = ' (justify %s)' % justify if justify else ''
        h = ' (hide yes)' if hide else ''
        return '(property "%s" "%s" (at %s %s %d) (effects (font (size 1.27 1.27))%s%s))' % (
            name, val, r2(x), r2(y), angle, j, h)

    def place(self, lib, ref, value, x, y, rot=0, fp='', refpos=None, valpos=None, hide_val=False):
        """place a symbol; ref None numbers a power symbol or flag. Returns the reference."""
        x, y = r2(x), r2(y)
        power = ref is None
        if power:
            pre = '#FLG' if lib == 'power:PWR_FLAG' else '#PWR'
            self.counters[pre] = self.counters.get(pre, 0) + 1
            ref = '%s%02d' % (pre, self.counters[pre])
        self.used.add(ref)
        for num, p in self.pins[lib].items():
            dx, dy = xform(p[0], p[1], rot)
            self.pinpos[(ref, num)] = (r2(x + dx), r2(y + dy))
        rp = refpos or (x + 2.54, y - 2.54)
        vp = valpos or (x + 2.54, y + 2.54)
        s = ['(symbol (lib_id "%s") (at %s %s %d) (unit 1) (exclude_from_sim no) (in_bom yes) (on_board yes) (dnp no)'
             % (lib, x, y, rot)]
        s.append('(uuid "%s")' % self.uid())
        fang = rot if rot in (90, 270) else 0
        fjust = 'left' if not power and not fang else None
        s.append(self.prop('Reference', ref, rp[0], rp[1], hide=power, angle=fang, justify=fjust))
        vjust = fjust
        if power and valpos is None and lib != 'power:PWR_FLAG':
            # VCC points up and GND down at rotation 0
            d = {0: (0, -1), 90: (-1, 0), 180: (0, 1), 270: (1, 0)}[rot]
            if lib == 'power:GND':
                d = (-d[0], -d[1])
            vp = (x + d[0] * 5.08, y + d[1] * 3.81)
            vjust = None
        s.append(self.prop('Value', value, vp[0], vp[1], hide=hide_val, angle=fang, justify=vjust))
        s.append(self.prop('Footprint', fp, x, y, hide=True))
        s.append(self.prop('Datasheet', '~', x, y, hide=True))
        s.append(self.prop('Description', '', x, y, hide=True))
        for num in self.pins[lib]:
            s.append('(pin "%s" (uuid "%s"))' % (num, self.uid()))
        s.append('(instances (project "%s" (path "/%s" (reference "%s") (unit 1))))' % (self.project, self.root, ref))
        s.append(')')
        self.items.append('\n'.join(s))
        return ref

    def P(self, ref, num):
        return self.pinpos[(ref, str(num))]

    def wire(self, a, b):
        self.items.append('(wire (pts (xy %s %s) (xy %s %s)) (stroke (width 0) (type default)) (uuid "%s"))'
                          % (r2(a[0]), r2(a[1]), r2(b[0]), r2(b[1]), self.uid()))

    def chain(self, pts):
        for a, b in zip(pts, pts[1:]):
            self.wire(a, b)

    def junction(self, p):
        self.items.append('(junction (at %s %s) (diameter 0) (color 0 0 0 0) (uuid "%s"))'
                          % (r2(p[0]), r2(p[1]), self.uid()))

    def nc(self, p):
        self.items.append('(no_connect (at %s %s) (uuid "%s"))' % (r2(p[0]), r2(p[1]), self.uid()))

    def glabel(self, name, p, side, shape='bidirectional'):
        """global label; side 'R': its body extends right of p, 'L' left, 'U' up, 'D' down"""
        ang = {'R': 0, 'L': 180, 'D': 270, 'U': 90}[side]
        just = 'right' if side in 'LD' else 'left'
        self.items.append(
            '(global_label "%s" (shape %s) (at %s %s %d) (fields_autoplaced yes) '
            '(effects (font (size 1.016 1.016)) (justify %s)) (uuid "%s") '
            '(property "Intersheetrefs" "${INTERSHEET_REFS}" (at %s %s 0) '
            '(effects (font (size 1.27 1.27)) (justify %s) (hide yes))))'
            % (name, shape, r2(p[0]), r2(p[1]), ang, just, self.uid(), r2(p[0]), r2(p[1]), just))

    def label(self, name, p, side='R'):
        """local net label"""
        ang = {'R': 0, 'L': 180, 'D': 270, 'U': 90}[side]
        just = 'right' if side in 'LD' else 'left'
        self.items.append('(label "%s" (at %s %s %d) (fields_autoplaced yes) '
                          '(effects (font (size 1.27 1.27)) (justify %s bottom)) (uuid "%s"))'
                          % (name, r2(p[0]), r2(p[1]), ang, just, self.uid()))

    def text(self, s, x, y, size=1.27):
        self.items.append('(text "%s" (exclude_from_sim no) (at %s %s 0) (effects (font (size %s %s)) '
                          '(justify left)) (uuid "%s"))' % (s, r2(x), r2(y), size, size, self.uid()))

    def vcc(self, p, rot=0):
        self.place('power:VCC', None, 'VCC', p[0], p[1], rot)

    def gnd(self, p, rot=0):
        self.place('power:GND', None, 'GND', p[0], p[1], rot)

    def stub(self, p, dx, dy):
        """wire from p by (dx, dy); returns the far end"""
        q = (r2(p[0] + dx), r2(p[1] + dy))
        self.wire(p, q)
        return q

    # ------------------------------------------------------------ CPU helpers
    def supply_rows(self, ref, cpu, top='VCC', bottom='GND'):
        """join the visible top pins to one rail and the bottom pins to another;
        a row entry of type power_out is left for the caller"""
        for row, net, d in ((cpu.top, top, -G), (cpu.bottom, bottom, G)):
            pts = sorted({self.P(ref, e[1][0]) for e in row if (len(e) < 3 or e[2] != 'power_out')})
            if not pts:
                continue
            self.chain(pts)
            for p in pts[1:-1]:
                self.junction(p)
            mid = pts[(len(pts) - 1) // 2]
            end = self.stub(mid, 0, d)
            (self.vcc if net == 'VCC' else self.gnd)(end)

    def label_pins(self, ref, cpu, labels, shapes, skip=()):
        """global labels on CPU side pins listed in labels {number: name};
        pins in skip are left for the caller, the rest get no-connect flags"""
        left = {p[1] for p in cpu.left if p}
        for side in (cpu.left, cpu.right):
            for p in side:
                if not p:
                    continue
                num = p[1]
                pos = self.P(ref, num)
                if num in labels:
                    self.glabel(labels[num], pos, 'L' if num in left else 'R', shapes.get(labels[num], 'bidirectional'))
                elif num not in skip:
                    self.nc(pos)

    def tie(self, ref, num, net, side):
        """tie a pin straight to VCC or GND; side is the direction the pin points"""
        rot = {'L': 90, 'R': 270, 'U': 0, 'D': 180}[side]
        if net == 'VCC':
            self.vcc(self.P(ref, num), rot)
        else:
            self.gnd(self.P(ref, num), {'L': 270, 'R': 90, 'U': 180, 'D': 0}[side])

    # ------------------------------------------------------------ Bionic frame
    def connectors(self, teensy, shapes, x=213.36, y=78.74):
        """J1 (Bionic-P135) and J2 (Bionic-P245) back to back; teensy maps 'P10'.. to label names.
        P57 is left for id_eeprom()."""
        self.place('bionic:Bionic-P135', 'J1', 'Bionic-P135', x, y, 0, 'bionic:Bionic-P135_SMT',
                   refpos=(x - 5.08, y - 31.75), valpos=(x - 5.08, y + 33.02))
        self.place('bionic:Bionic-P245', 'J2', 'Bionic-P245', x + 3.81, y, 0, 'bionic:Bionic-P245_SMT',
                   refpos=(x + 1.27, y - 31.75), valpos=(x - 5.08, y + 35.56))
        for ref, lib, side in (('J1', 'bionic:Bionic-P135', 'L'), ('J2', 'bionic:Bionic-P245', 'R')):
            d = -G if side == 'L' else G
            for num, p in self.pins[lib].items():
                pos = self.P(ref, num)
                nm = p[4]
                if nm == 'VCC':
                    self.vcc(self.stub(self.stub(pos, d, 0), 0, -G))
                elif nm == 'GND':
                    self.gnd(self.stub(self.stub(pos, d, 0), 0, G))
                elif nm == 'P57':
                    pass
                elif nm in teensy:
                    self.glabel(teensy[nm], pos, side, shapes.get(teensy[nm], 'bidirectional'))
                else:
                    self.nc(pos)

    def id_eeprom(self, ref='U2'):
        """11AA010 straight across from J2's P57"""
        p57 = self.P('J2', 26)
        ex = p57[0] + 20.32
        self.place('microchip:11AA010-I_TT', ref, '11AA010-I/TT', ex + 2.54, p57[1] - 1.27, 0,
                   'microchip:SOT-23_MC_MCH-M', refpos=(ex + 12.7, p57[1] - 1.27), valpos=(ex + 12.7, p57[1] + 1.27))
        self.wire(p57, self.P(ref, 1))
        self.vcc(self.stub(self.P(ref, 2), -G, 0), 90)
        self.gnd(self.stub(self.P(ref, 3), -G, 0), 270)

    def led_and_flags(self, led='D1', res='R1', x=271.78, y=30.48, fx=248.92, fy=33.02):
        self.vcc((x, y))
        self.place('rhom:CSL1901UW1', led, 'CSL1901UW1', x, y, 270, 'rhom:LED_CSL1901UW1_ROM-M',
                   refpos=(x + 5.08, y + 2.54), valpos=(x + 8.89, y + 5.08))
        self.place('Device:R_Small', res, '1.6k', x, y + 12.7, 0, R0603,
                   refpos=(x + 2.54, y + 14.605), valpos=(x + 2.54, y + 16.51))
        self.wire(self.P(led, 1), self.P(res, 1))
        self.gnd(self.P(res, 2))
        self.vcc((fx, fy))
        self.place('power:PWR_FLAG', None, 'PWR_FLAG', fx, fy, 180, valpos=(fx, fy + 4.0))
        self.gnd((fx + 7.62, fy))
        self.place('power:PWR_FLAG', None, 'PWR_FLAG', fx + 7.62, fy, 0, valpos=(fx + 7.62, fy - 4.0))

    def cap(self, value, x, y, top='VCC', ref=None):
        """one capacitor; top is VCC or a local net label name, the bottom goes to GND"""
        ref = ref or self.next_ref('C')
        self.place('Device:C_Small', ref, value, x, y, 0, C0603, refpos=(x + 1.27, y - 1.27), valpos=(x + 1.27, y + 1.27))
        if top == 'VCC':
            self.vcc(self.P(ref, 1))
        else:
            self.label(top, self.P(ref, 1), 'U')
        self.gnd(self.P(ref, 2))
        return ref

    def cap_row(self, values, x=195.58, y=146.05):
        """decoupling caps between VCC and GND, wired in a row"""
        refs = []
        for i, v in enumerate(values):
            ref = self.next_ref('C')
            px = x + i * 5.08
            self.place('Device:C_Small', ref, v, px, y, 0, C0603,
                       refpos=(px - 1.27, y - 5.08), valpos=(px - 1.27, y + 5.08))
            refs.append(ref)
        for pin, sym in ((1, self.vcc), (2, self.gnd)):
            pts = [self.P(r, pin) for r in refs]
            self.chain(pts)
            for p in pts[1:-1]:
                self.junction(p)
            sym(self.stub(pts[0], -5.08, 0))
        return refs

    def resistor(self, value, x, y, rot=0, ref=None):
        ref = ref or self.next_ref('R')
        self.place('Device:R_Small', ref, value, x, y, rot, R0603,
                   refpos=(x + 1.27, y - 0.635), valpos=(x + 1.27, y + 1.27))
        return ref

    # ------------------------------------------------------------ output
    def write(self, path, title, date, comment='', rev='1', paper='A4'):
        doc = ('(kicad_sch (version 20250114) (generator "eeschema") (generator_version "9.0") (uuid "%s") '
               '(paper "%s") (title_block (title "%s") (date "%s") (rev "%s") (comment 1 "%s")) '
               '(lib_symbols\n%s\n)\n%s\n(sheet_instances (path "/" (page "1"))) (embedded_fonts no))'
               % (self.root, paper, title, date, rev, comment, '\n'.join(self.libs.values()), '\n'.join(self.items)))
        with open(path, 'w') as f:
            f.write(format_text(doc))

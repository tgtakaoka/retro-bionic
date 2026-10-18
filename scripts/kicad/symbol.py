"""Draw a CPU schematic symbol from pin lists.

Signal pins go on the left and right edges, one per 2.54 mm row; None
leaves a gap. Supply pins go along the top and bottom; a supply name with
several pins stacks them on one point, the first visible, the rest hidden.
Unused pins are hidden no-connects.
"""
G = 2.54


class CpuSymbol:
    def __init__(self, name, npins, left, right, top=(), bottom=(), hidden=(), width=45.72,
                 footprint='', datasheet='', description='', keywords='', fp_filter=''):
        """left, right: [(name, number, type) or None]
        top, bottom: [(name, (numbers...)) or (name, (numbers...), type)]; type defaults to power_in
        hidden: [(name, number)]"""
        self.name = name
        self.npins = npins
        self.left, self.right = list(left), list(right)
        self.top, self.bottom, self.hidden = list(top), list(bottom), list(hidden)
        self.W = width
        self.footprint, self.datasheet = footprint, datasheet
        self.description, self.keywords, self.fp_filter = description, keywords, fp_filter
        rows = max(len(self.left), len(self.right))
        self.YTOP = 8.89                         # body top above row 0
        self.YBOT = -(rows - 1) * G - 7.62
        self.check()

    def _centred(self, n):
        return round((self.W / 2 - (n - 1) * G / 2) / 1.27) * 1.27

    def pins(self):
        """yield (name, number, type, x, y, angle, hide) in symbol coordinates"""
        for i, p in enumerate(self.left):
            if p:
                yield p[0], p[1], p[2], -G, -i * G, 0, False
        for i, p in enumerate(self.right):
            if p:
                yield p[0], p[1], p[2], self.W + G, -i * G, 180, False
        for row, y, a in ((self.top, self.YTOP + G, 270), (self.bottom, self.YBOT - G, 90)):
            x0 = self._centred(len(row))
            for i, entry in enumerate(row):
                n, nums = entry[0], entry[1]
                typ = entry[2] if len(entry) > 2 else 'power_in'
                for k, num in enumerate(nums):
                    yield n, num, typ if k == 0 else 'passive', x0 + i * G, y, a, k > 0
        for i, (n, num) in enumerate(self.hidden):
            yield n, num, 'no_connect', 7.62 + (i % 12) * G, -20.32 - (i // 12) * G, 0, True

    def check(self):
        nums = sorted(p[1] for p in self.pins())
        assert nums == list(range(1, self.npins + 1)), \
            [n for n in range(1, self.npins + 1) if nums.count(n) != 1]

    def text(self, prefix='cpu:'):
        f = '(effects (font (size 1.27 1.27)))'
        name = prefix + self.name
        hid = '(effects (font (size 1.27 1.27)) (hide yes))'
        out = ['(symbol "%s"' % name,
               '(pin_names (offset 1.016))',
               '(exclude_from_sim no) (in_bom yes) (on_board yes)',
               '(property "Reference" "U" (at 0 %.2f 0) (effects (font (size 1.27 1.27)) (justify left bottom)))'
               % (self.YTOP + 3.81),
               '(property "Value" "%s" (at %.2f %.2f 0) (effects (font (size 1.27 1.27)) (justify top)))'
               % (self.name, self.W / 2, self.YBOT - 5.08),
               '(property "Footprint" "%s" (at 0 0 0) %s)' % (self.footprint, hid),
               '(property "Datasheet" "%s" (at 0 0 0) %s)' % (self.datasheet, hid),
               '(property "Description" "%s" (at 0 0 0) %s)' % (self.description, hid),
               '(property "ki_keywords" "%s" (at 0 0 0) %s)' % (self.keywords, hid),
               '(property "ki_fp_filters" "%s" (at 0 0 0) %s)' % (self.fp_filter, hid),
               '(symbol "%s_0_1" (rectangle (start 0 %.2f) (end %.2f %.2f) (stroke (width 0.254) (type default)) '
               '(fill (type background))))' % (self.name, self.YTOP, self.W, self.YBOT),
               '(symbol "%s_1_1"' % self.name]
        for n, num, typ, x, y, a, hide in self.pins():
            out.append('(pin %s line (at %.2f %.2f %d) (length 2.54)%s (name "%s" %s) (number "%d" %s))'
                       % (typ, x, y, a, ' (hide yes)' if hide else '', n, f, num, f))
        out += [')', '(embedded_fonts no)', ')']
        return '\n'.join(out)

    def library(self):
        """a standalone cpu.kicad_sym holding this symbol"""
        return ('(kicad_symbol_lib (version 20241209) (generator "kicad_symbol_editor") (generator_version "9.0")\n'
                + self.text(prefix='') + '\n)\n')

    def pin_table(self):
        """number -> (x, y, angle, length, name, type), as sexp.pins_of returns"""
        return {str(num): (x, y, a, G, n, typ) for n, num, typ, x, y, a, hide in self.pins()}

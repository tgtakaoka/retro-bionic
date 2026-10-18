"""Write a board's <chip>_bionic.toml from its netlist and the chip's pinout toml.

Each CPU pin line becomes "<Teensy pin>, <pinout names>" when a net joins
it to J1/J2, "VCC, ..." or "GND, ..." for supply ties, and ",    ..."
otherwise.

    bionic_toml.py map <netlist>

prints the Teensy pin -> net table, to check against the Bionic Connector
sheet column.
"""
import re
import sys

from pcb import read_netlist

P135 = {1: 'GND', **{2 + i: 'P1%d' % i for i in range(8)}, **{11 + i: 'P3%d' % i for i in range(8)},
        **{20 + i: 'P5%d' % i for i in range(4)}, 24: 'VCC'}
P245 = {25: 'VCC', 26: 'P57', 27: 'P56', 28: 'P55', 29: 'P54', **{31 + i: 'P4%d' % (7 - i) for i in range(8)},
        **{40 + i: 'P2%d' % (7 - i) for i in range(8)}, 48: 'GND'}


def teensy_pin(nodes):
    for r, p in nodes:
        if r == 'J1':
            return P135[int(p)]
        if r == 'J2':
            return P245[int(p)]
    return None


def teensy_map(netlist):
    """Teensy pin -> net name"""
    _, nets = read_netlist(netlist)
    out = {}
    for name, nodes in nets.items():
        for r, p in nodes:
            pin = P135.get(int(p)) if r == 'J1' else P245.get(int(p)) if r == 'J2' else None
            if pin and pin not in ('VCC', 'GND') and not name.startswith('unconnected-'):
                out[pin] = name.lstrip('/')
    return out


def pinout_names(path):
    fn = {}
    for line in open(path):
        m = re.match(r'\s*(\d+)\s*=\s*"([^"]+)"', line)
        if m:
            fn[int(m.group(1))] = re.sub(r',\s+', ', ', m.group(2))
    return fn


def write(netlist, pinout, out, header, npins, cpu='U1', resolve=None):
    """header: the lines before the pin table; resolve(nodes, nets) may name a
    Teensy pin reached through other parts (a mux, a buffer)"""
    _, nets = read_netlist(netlist)
    pin_net = {int(p): (name, nodes) for name, nodes in nets.items() for r, p in nodes if r == cpu}
    fn = pinout_names(pinout)
    lines = []
    for n in range(1, npins + 1):
        name, nodes = pin_net.get(n, ('', []))
        c = teensy_pin(nodes) or (resolve(nodes, nets) if resolve else None)
        if c is None and name in ('VCC', 'GND'):
            c = name
        lines.append('%3d = "%s"' % (n, ('%s, %s' % (c, fn[n])) if c else ',    %s' % fn[n]))
    with open(out, 'w') as f:
        f.write(header + '\n'.join(lines) + '\n')


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] != 'map':
        sys.exit(__doc__)
    m = teensy_map(sys.argv[2])
    for port in range(1, 6):
        for bit in range(8):
            k = 'P%d%d' % (port, bit)
            if k in m:
                print('%s\t%s' % (k, m[k]))

#!/usr/bin/env python3
"""Saleae Logic Pro / Logic 2 specific tooling -- not part of the generic
LogicAnalyzer interface in logic-analyzer.py.

  load-preset PRESET [SET_NAME]   dump a Logic 2 preset's channel labels
                                   as a TOML table named SET_NAME (default
                                   PRESET), to redirect or append into
                                   samples/<target>/bench/channels.toml

The preset is read from Logic 2's config file, $BIONIC_LOGIC_CONFIG
(default ~/.config/Logic/config.json).
"""
import os
import sys
from importlib.machinery import SourceFileLoader

HERE = os.path.dirname(os.path.abspath(__file__))
la = SourceFileLoader('la', os.path.join(HERE, 'logic-analyzer.py')).load_module()


def _toml_string(s):
    return '"%s"' % s.replace('\\', '\\\\').replace('"', '\\"')


def load_preset_toml(preset, set_name=None):
    """A Logic 2 preset's channel labels, as a TOML table."""
    channels = la.SaleaeLogicPro16().preset_channels(preset)
    lines = ['[%s]' % (set_name or preset)]
    for ch, name in sorted(channels.items()):
        lines.append('%d = %s' % (ch, _toml_string(name)))
    return '\n'.join(lines) + '\n'


def main():
    a = sys.argv[1:]
    if a and a[0] == 'load-preset' and len(a) > 1:
        print(load_preset_toml(a[1], a[2] if len(a) > 2 else None), end='')
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()

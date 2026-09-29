"""HD6301 or HD6303, which prefetch: cycles_mc6800.py with hd6301.txt, for scripts/record-cycles.py."""
import importlib.machinery
import os

_base = importlib.machinery.SourceFileLoader(
    'cycles_mc6800', os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                  'cycles_mc6800.py')).load_module()
_base.define(globals(), 'hd6301', ('HD6301',), internal=0x100, prefetch=True, regs16=True)

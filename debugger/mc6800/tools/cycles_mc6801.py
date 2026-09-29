"""MC6801 or MC6803: cycles_mc6800.py with mc6801.txt, for scripts/record-cycles.py."""
import importlib.machinery
import os

_base = importlib.machinery.SourceFileLoader(
    'cycles_mc6800', os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                  'cycles_mc6800.py')).load_module()
_base.define(globals(), 'mc6801', ('MC6801',), internal=0x100, regs16=True)

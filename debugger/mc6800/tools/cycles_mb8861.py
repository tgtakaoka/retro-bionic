"""MB8861 on the MC6800 or MC6802 board: cycles_mc6800.py with mb8861.txt, for scripts/record-cycles.py."""
import importlib.machinery
import os

_base = importlib.machinery.SourceFileLoader(
    'cycles_mc6800', os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                  'cycles_mc6800.py')).load_module()
_base.define(globals(), 'mb8861', ('MB8861', 'MB8870'), internal=0x80)

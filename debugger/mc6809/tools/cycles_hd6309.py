"""HD6309 plugin for scripts/record-cycles.py, for an HD6309 on either the
MC6809 or the MC6809E board: cycles_mc6809.py with the hd6309-*.txt
tables, every pattern run in emulation mode and again in native mode
(MD=1, keys ending in :native).

    P=debugger/mc6809/tools/cycles_hd6309.py
    R=debugger/mc6809/tools/hd6309-cycles.jsonl.zst
"""
import importlib.machinery
import os

_base = importlib.machinery.SourceFileLoader(
    'cycles_mc6809', os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                  'cycles_mc6809.py')).load_module()
_base.define(globals(), 'hd6309', native=True)
# The board reports the chip it found.
NAME = ('HD6309', 'HD6309E')

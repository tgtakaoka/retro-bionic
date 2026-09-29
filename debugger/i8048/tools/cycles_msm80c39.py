"""OKI MSM80C35/MSM80C39 (and MSM80C48) on the P8048 board:
cycles_i8048.py with the OKI opcodes, for scripts/record-cycles.py.

    P=debugger/i8048/tools/cycles_msm80c39.py
    R=debugger/i8048/tools/msm80c39-cycles.jsonl.zst
    scripts/record-cycles.py $P fill
    scripts/record-cycles.py $P run --record $R
    scripts/record-cycles.py $P check --record $R
"""
import importlib.machinery
import os

_base = importlib.machinery.SourceFileLoader(
    'cycles_i8048', os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                 'cycles_i8048.py')).load_module()
_base.define(globals(), ('MSM80C39',), _base.F_I8039 | _base.F_80C39 | _base.F_MSM39)

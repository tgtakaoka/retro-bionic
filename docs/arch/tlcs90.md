# TLCS90

Part of [Development style](../development-style.md): read General first.

**Pitfalls.** On-chip RAM (FEC0-FFBF) and I/O (FFC0-FFEF) never show on
the bus, so instructions touching them make fewer cycles than their rows;
FFF0-FFFF is external again **[hw]**. A word access at FFFF wraps to 0000.

The profile (`tools/cycles_tlcs90.py`, recorded as
`tools/tlcs90-cycles.jsonl.zst`) matches the manual's bus-cycle tables on
every opcode. **[hw]**

- `JRL` and `CALR` count their 16-bit displacement from the opcode + 2, as
  an 8-bit `JR`'s does.
- `r` and `w` allow the wrap at FFFF.

The profile keeps its operands and SP in external memory, and leaves out
patterns that would point SP at on-chip I/O. **[hw]**

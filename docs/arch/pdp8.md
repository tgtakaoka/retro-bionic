# PDP-8 (IM6100 / HD6120)

Part of [Development style](../development-style.md): read General first.

**Pitfalls.** The debugger's `restore()` ends in `RTF` on both chips, and
on the IM6100 `RTF` enables interrupts after the next instruction whatever
IEFF was. **[hw]** Resuming inside an interrupt handler whose request is
still pending would take the interrupt again and overwrite the return
address in location 0000. The debugger holds INTREQ off while the saved
IEFF is clear, records IEFF as clear in `save()` until then, and lets
requests through once the program fetches its own `ION` or `RTF`. The
HD6120 keeps interrupts off until the handler's own return. To check,
stop inside `echoir`'s handler with the request pending, step and `G`, and
see that location 0000 keeps the main-line return address.

Holding INTREQ off still leaves the chip's own IEFF on, which `SKON` and
`GTF` read. `restore()` follows the `RTF` with `IOF` on both chips when the
saved IEFF is clear: `RTF` enables interrupts one instruction late, so the
`IOF` turns them off before any request is taken. To check, halt and resume
an `IOF`, `SKON`, `JMP .-1` loop: it must never fall through to a `HLT`
after it. **[hw]**

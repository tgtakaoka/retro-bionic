# Development style

Hard-won notes from bringing up targets in this codebase. Things that cost real
debugging time and are not obvious from the source.

Each claim is marked **[hw]** when it was verified on the bench, **[doc]** when it
comes from a datasheet or manual, and **[code]** when it is a property of this
codebase.

---

## General

### Injection models: sequential cursor vs address-keyed

There are two ways to answer a CPU's reads from an injected byte stream, and they
are not interchangeable:

- **Sequential cursor** — every read consumes the next byte(s) of the stream.
  Works when the CPU reads exactly what it consumes, one byte at a time.
- **Address-keyed** — a read at address *A* is answered from `inst[A - origin]`,
  like memory would.

A CPU that re-reads at every PC value, or fetches words while consuming bytes,
desynchronises a cursor immediately: it takes two bytes per read but advances the
PC by one, so the stream is handed out at twice the rate it is consumed. **[hw]**

This matters because the idioms differ. The Z80 "faked POP" — a `POP rr` opcode
followed inline by the payload bytes — only works under a cursor, where the
stack read naturally takes the next two bytes. Under address-keyed injection a
stack read lands at SP, nowhere near the window, and is answered from real
memory instead. **[hw]**

Under address-keyed injection, prefer real load instructions over faked POPs.
Where a register genuinely has no load (a flags register, typically), stage the
value in memory and point SP at it.

### Terminating an injected sequence

Do not count bytes handed over. One word read can deliver two single-byte
instructions, so a byte count reaches the end of the stream while the CPU still
has an address left to ask about — and that read gets answered from memory,
handing the CPU an opcode that is not in the sequence at all. **[hw]**

Terminate on *the CPU having read past the window* instead.

The remaining hazard is that the CPU prefetches past the end of a window before
it takes a trailing jump, and that prefetch is indistinguishable from a genuine
forward exit. If a sequence must end by jumping somewhere specific (back to its
own origin, or to a restored PC), the caller has to say so; it cannot be
recovered from the bus trace. **[hw]**

### Pipeline flushes force re-fetches

Any instruction that flushes the prefetch queue makes the CPU re-fetch addresses
it has already passed. So does interrupt and trap processing. An injected window
must stay answered across those re-fetches, not just until its last byte has
gone out — a re-fetch answered from memory is garbage the CPU then executes.
**[hw]**

The same applies to an injected interrupt handler: the vector is fetched, the
pipeline flush discards it, and the CPU fetches it again. Serve the vector for
as long as the CPU asks for it. **[hw]**

### `Cycles::reset()` and the parked transaction **[code]**

Targets park the CPU by leaving a transaction incomplete and recording it in the
ring head; `resumeCycle()` reads that slot back to finish it. `Cycles::reset()`
used to clear the head slot, destroying the parked transaction's address — every
resumed transaction then reported address zero, which in turn set an injection
origin of zero and corrupted everything downstream.

`Cycles::reset()` now carries the parked slot over instead of clearing it. Watch
for this whenever a target resets the ring between `restore()` and stepping.

### Keep bus-keepalive cycles out of the ring **[code]**

Refresh transactions are the bus keeping DRAM alive, not the program doing
anything, and at debugger clock speeds one lands between almost every pair of
real transactions. Recording them:

- breaks `s->prev()`, which callers use to mean "the previous *program*
  transaction" — e.g. matching an interrupt vector fetch against the PC push
  that must immediately precede it;
- floods the 128-entry ring so a dump shows nothing else.

`completeCycle()` should not advance the ring for them. This is the fix for
refresh noise — not disabling refresh at the source (see Z280 below).

### Bound every wait loop **[hw]**

`loop()` only polls the halt switch *between* steps. A wait loop that never
returns cannot be broken into from the halt port, and wedges the board past
recovery — reflashing is the only way back. Give every "wait for the CPU to do
X" loop a guard and a failure path.

This bit twice: an unbounded refresh-skip in `prepareCycle()`, and an unbounded
NMI-acknowledge wait in `suspend()`.

### The halt port **[hw]**

The Teensy builds with `USB_DUAL_SERIAL`. `/dev/ttyACM0` is the debugger
console; **any byte written to `/dev/ttyACM1` aborts a running CPU**
(`serialEventUSB1()` → `Pins::isrHaltSwitch()` → `_halted`, polled by `loop()`).
It is the only way to stop a run that does not end on its own — provided the
wait loops are bounded.

Two things about it cost a lot of time before they were understood.

**The flag used to re-arm itself.** `serialEventUSB1()` raised `_halted` but
never *read* the byte, and the Teensy core re-calls the handler from every
`yield()` while the port still holds data. `setRun()` cleared the flag and the
next `yield()` set it again, so **one abort also killed the following run** —
`Debugger::go()` was the only thing that drained the port, at the end of a run,
one run too late. The symptom is a board that looks wedged: every `G` returns
instantly with a register dump. Fixed by draining in the handler (PR #38). **[hw]**

**An abort leaves the emulated USART repeating.** After a run is stopped
mid-cycle, the USART keeps re-delivering its last received character, which
floods `ttyACM0` and buries the CLI prompt. A harness must drain the console
after any abort, and should prefer to stop an interactive sample the way the
sample itself expects (NUL for `samples/z280`) over aborting it. **[hw]**

A working recovery order for an unresponsive board, in increasing violence:
Ctrl-C (`0x03`), Ctrl-Space (`0x00`), then the halt port. If none of the three
draws a reply, the firmware itself is stuck in an unbounded wait and only a
reflash will bring it back.

### Compare a pin read against `LOW`, never `HIGH` **[code]**

`digitalReadFast()` is not guaranteed to normalise to 1 — a fast read can hand
back the masked register bit, `1 << bit`. That is truthy but **not equal to
`HIGH`**, so `read() == HIGH` can be false while the pin is high, and the bug
appears only on the pins whose bit position is not 0. `LOW` is 0, so `== LOW`
and `!= LOW` are exact whatever the accessor returns. Write the test that way
even when the current expansion happens to be safe.

### A logic analyser on the bus can break the board **[hw]**

Sixteen probe leads on the multiplexed bus (AD, status, `#AS`, `#DS`, `#WAIT`,
`#RESET`, R/`#W`) add enough capacitance to stop a marginal target working at
all. Measured: a commit that had just passed all five `samples/z280` programs
went to *zero* passes, unrecoverable every time, with no change to the source —
purely from the probes being attached. Unplugging them restored it.

Two consequences:

- **Bisect against a known-good commit before believing any code theory.** With
  the probes on, every build failed, so each change tested in isolation looked
  like the culprit. Hours went into a `Cycles::reset()` change, a reset setup
  delay, a `clk_delay_ns` margin and an `#AS` polling rewrite, none of which
  were the fault. Checking out the last commit known to pass would have ended it
  immediately. `git reflog --date=iso` dates the builds, so the timestamps of a
  session's passing run identify which commit to try.
- **Timing measured with the probes on is not the unloaded timing.** Absolute
  setup and hold figures taken that way describe a bus that no longer behaves
  like the one shipping.

### Bus level holders make the data bus turnaround uncritical **[hw]**

A hardware design decision worth knowing before optimising any timing around
it. The port 6 pins **P6.16 to P6.31** — the data bus, among other uses — each
carry a **bus level holder**. When the controller switches one of those pins
from output to input, the level it was driving is *held* on the net until the
CPU actually drives the pin itself.

So the data bus never floats across the turnaround. The window between
releasing the bus (`inputMode()`) and the CPU taking it over, which would
otherwise be the tightest timing in a read cycle, is covered by the holder: a
CPU still sampling during it reads the level the controller last drove, not an
indeterminate one.

Two things follow:

- Releasing the bus does not have to be timed precisely against the point where
  the CPU stops sampling. Effort spent shaving that margin is wasted.
- A read of a pin in that range tells you what is *held* there, which is not
  evidence that the CPU is driving it. Do not infer bus ownership from a level.

### Verify opcodes with the assembler, not by hand

libasm ships a CLI assembler; use it rather than hand-encoding injected
sequences, especially for relative jump displacements:

```
printf '        CPU Z280\n        ORG 0\n        JR 0000H\n' > /tmp/x.asm
/home/t2/libasm/cli/asm -C z280 -l /dev/stdout /tmp/x.asm -o /dev/null
```

It also settles opcode questions directly from the tables
(`libasm/src/table_<cpu>.cpp`).

### Console gotchas when loading a test program **[hw]**

- `M`/`m` (program/data memory) accept at most **16 bytes per line**. A longer
  program truncates silently, and the untouched tail then executes as whatever
  was already there — which reads as "the CPU got that instruction wrong". Split
  the load across several `M` commands and dump it back before drawing any
  conclusion.
- `M`/`m` always *write*; `p`/`d` dump. `M<addr>` followed by an empty line still
  writes, so a mistyped dump can quietly corrupt the byte you were about to
  inspect.
- `b` (list breakpoints) prompts "clear?" and will swallow whatever command comes
  next. `B<addr>` sets one without prompting.

### Read the manual before blaming the code

Several long detours would have been avoided by grepping the manual first.
`pdftotext -layout` on the PDF makes it greppable. Specifically: a measured
behaviour that contradicts an assumption is usually documented somewhere, and a
"this should work" fix that does not work usually has a paragraph explaining
why.

---

## Per architecture

### Z280

**Z-BUS byte lanes.** The even-address byte rides AD8-15 and the odd-address
byte AD0-7 (§13.5.1.1). The lane is chosen by the **absolute address**, not by
an offset into a buffer. The two agree only while a sequence sits at an even
origin; at an odd origin the whole stream is delivered shifted by one byte.
This applies equally to injection and to `MemsZ280::read_zbus`/`write_zbus`.
**[hw]**

Word transfers are *not* always even-aligned: with caching off the CPU reads a
word at every PC value, so it asks on both parities. **[hw]**

**On-chip memory is a cache at reset.** Cache Control is control register `12`,
reset value `20` (M/C=0, I=0, D=1) — instruction caching on. The debugger never
sees a fetch the cache answers, so the bus falls silent.

- `I=1` stops *new* lines being filled but does **not** invalidate what is
  already cached. A short loop that was cached before `I=1` took effect keeps
  running entirely from cache. **[hw]**
- `PCACHE` (`ED 65`) drops the existing lines. **[hw]**
- Fixed-address mode (`M/C=1`) also frees the bus but needs all 16 line tags
  initialised by data reads first, which injection cannot carry — there is no
  `#M1` to tell a data read from a fetch. **[hw]**

Uncached, the CPU issues a word read at every PC value (instruction
boundaries), not one per two bytes. **[hw]**

**Refresh cannot be turned off.** Clearing the Refresh Enable bit does not stop
the transactions; §9.3 repurposes them as a *minimum bus transaction rate*: "if
the refresh timer reaches 0 and no external bus transaction has occurred since
the last time the refresh timer elapsed, then a refresh transaction will be
generated." A parked or slowly-stepped CPU is exactly that condition. The rate
field does not remove them either — at the speed the debugger clocks the part,
hundreds of processor clocks pass per bus transaction, so the timer has always
elapsed. Measured: the refresh-to-MEM ratio was unchanged before and after.
**[hw][doc]**

That does *not* mean leaving the register alone. The manual requires the rate
field to hold a sensible value even with refresh disabled, since it is what
governs the keep-alive interval. **Rate 0 is not the slowest setting** — the
field is documented over `0 < n < 63` (once every 4n clocks) with zero as a
separate special case, so the slowest is `3F`. Writing `00`, as a first attempt
did, sets the enable bit correctly but leaves the rate field at its degenerate
value. **[doc]**

So both are needed, for different reasons: set the register (`E=0`, rate `3F`)
because it is the correct configuration for a board with no DRAM, and keep
refresh out of the ring (see General) because the transactions still happen.

Whatever writes that register must put the **I/O Page register back to zero**
afterwards — see below.

**I/O Page register is cleared to zero by reset.** On-chip peripherals live on
pages `FE`/`FF`, so an `OUT` after reset goes to page `00` — an *external* I/O
transaction. Anything that changes the page must put it back, or every later I/O
transaction is handled on-chip and emits no bus cycle at all, which hangs a
debugger waiting for one. **[hw][doc]**

On-chip I/O generates no external bus transaction. **[doc]**

**Control register addresses** (`LDCTL`, address in C) **[doc]**:

| Register | Addr | | Register | Addr |
|---|---|---|---|---|
| Master Status (MSR) | `00` | | Trap Control \* | `10` |
| Bus Timing and Control \* | `02` | | Cache Control \* | `12` |
| Stack Limit | `04` | | Local Address \* | `14` |
| Interrupt/Trap Vector Table Pointer | `06` | | Interrupt Status | `16` |
| I/O Page \* | `08` | | Bus Timing and Initialization \* | `FF` |

\* 8-bit; only the low byte of the source register is written.

**Clocking.** `CLK = XTALI` with CS=01 in the Bus Timing and Initialization
register (CS=00 gives XTALI/2), ~16 ns XTALI→CLK propagation. Measured on a
memory transaction: `#AS` is asserted for one XTALI phase; `#DS` falls one phase
later on a read and two on a write, staying low three phases and two
respectively. **[hw]**

**The CS latch does not take effect until the first bus transaction.** While
`#RESET` is asserted, and on past it, `CLK` stays on a fixed reset divider
regardless of what was just latched into CS. So the latch cannot be verified in
the `#WAIT` hold window after `#RESET` is released — measured there, `CLK`
changes 7 times in 28 samples whether CS was latched `01` or `00`, and a check
for "steady" can never pass. `resetPins()` had exactly that check and therefore
ran all of its retries, every time, without ever testing anything.

Measured *past* `prepareCycle()` — that is, once the first bus transaction has
begun — it is a clean discriminator: sampling `CLK` once per `#XTALI` cycle
gives **0 changes in 16 at CS=01 and 12 at CS=00**. The verification belongs
there. It is safe to spend cycles at that point because `#WAIT` parks the CPU
in T2; it is *not* safe before, where extra cycles carry the CPU past T1 and
leave `prepareCycle()` waiting on an `#AS` that has already gone. **[hw]**

**Sample the address inside the `#AS`-low window, the status after the rise.**
AD0-15 carries the address only while `#AS` is low; the rise latches it and the
CPU then turns AD around for data. `prepareCycle()` sampled both 60ns *past* the
rise, so `getAddr()` could read whatever AD had become — the debugger then
reported nonsense addresses, and stopping a run gave a PC outside the program
(`A409`, `EE53`) with a different byte count every time. Sampling the address at
the end of the `#AS`-low window and only the status after the rise made runs
identical: `PC=021F`, 2232 bytes, 8/8. `#AS` cannot rise until #XTALI is driven
low, so waiting inside that window costs nothing and only lets AD settle.
**[hw]**

The 3-state status lines still need the rise, so the two samples straddle it:

```
while (signal_as() != LOW) { ...clock... }
delayNanoseconds(addr_delay_ns);   // settle, still inside the #AS-low window
s->getAddr();
xtali_lo();                        // ...which makes the #AS rise
delayNanoseconds(status_delay_ns);
s->getControl();
```

**`completeCycle()` cannot count T states.** It is sometimes entered straight
after `resumeCycle()`, which is resuming a transaction out of a `#WAIT` stretch,
and there is then no way to know which T state the CPU is in. So both edges have
to be found by watching `#DS`, never by clocking a fixed number of cycles from a
supposedly known one. Restructuring the write path to latch "one cycle past the
T2 `#DS` fall, at the T3 rise" — which is where the data really is valid — broke
every run for exactly this reason: after a resume that extra cycle lands
somewhere arbitrary. Getting the write sample later without tracking state
through `resumeCycle()` remains open. **[hw]**

**Sustained clock rate is set by host code, not by the delay constants.** Over a
whole `mandelbrot` run, `CLK` and `#XTALI` both average **3.52 MHz** (323,007
cycles in 91.75 ms; identical on both channels, confirming CS=01). The rate is
remarkably flat — median period 280 ns, max 352 ns, so there are no stalls to
average over — and the duty is **89% high**. That shape is `xtali_cycle_hi()`:
the low phase is just the 20 ns delay plus GPIO overhead, while the high phase
absorbs whatever `completeCycle()`, `_devs->loop()` and `prepareCycle()`
bookkeeping cost that cycle. The ~9 MHz reached during tuning is the bare toggle
rate of the `prepareCycle()` wait loop, which does nothing but toggle and read a
pin; it is not reachable while real work happens every cycle, and trimming the
delay constants further buys very little. **[hw]**

`ST0-3`, `R/#W`, `B/#W` and `#AS` are 3-state and valid only from the rising
edge of `#AS` — sample them with real margin past that edge or you read floating
pins. **[hw]**

**NMI** (interrupt modes 0, 1, 2): pushes **PC only** — the MSR is not saved —
and vectors to `0066H`. `RETN` (`ED 45`) returns. No vector table is needed.
Interrupt processing flushes the pipeline, so `0066H` is fetched more than once
and must stay served. The pushed value is the address of the next instruction,
except for block instructions, where it is the address of the block instruction
itself. **[hw][doc]**

`#NMI` must be asserted **during** the opcode fetch of the instruction to be
stepped over — after `resumeCycle()` hands that fetch back, but before
`completeCycle()` finishes it. The window is narrow in both directions:

- asserted after the fetch completes, the CPU does not see it in time for the
  following instruction boundary and steps **two** instructions;
- asserted before the parked transaction is really the opcode fetch, it vectors
  without running anything and steps **none**.

This is genuinely timing-sensitive, and the sensitivity is not theoretical:
cutting the refresh rate removed idle cycles that had been giving `#NMI` time to
be recognised, and a step that had been landing on exactly one instruction
started landing on two. It only became reliable once `resumeCycle()` was
guaranteed to return the opcode fetch itself, which needed the `exit` fix — the
`JP` at the tail of `LD_ALL` had been parking on a prefetch instead. **[hw]**

Compare `pins_tms9900_base.cpp`, which asserts on the first cycle where
`s->fetch()` is true; targets without an `#M1`-equivalent have to substitute
"the transaction `resumeCycle()` handed back". **[code]**

That a single-step mechanism can be knocked out by changing an unrelated refresh
divisor is the strongest practical argument for the trap route below.

**Prefetch reaches past an injected window.** The CPU reads addresses beyond the
end of a sequence before it takes a jump the sequence ends with. That prefetch is
indistinguishable on the bus from falling through, which is why each injected
sequence has to declare where it leaves off (`EXIT_END`, `EXIT_ORG`, or the
target address of its trailing jump) rather than the loop inferring it. **[hw]**

A corollary for breakpoints: stopping *before* a `HALT` is done by reading the
opcode out of memory, not by watching the bus, so a prefetch queue filled before
the `HALT` was written can still carry the CPU into it. **[hw]**

#### `loop()` must free-run, not single-step **[hw]**

`run()`'s loop originally called `rawStep()` per instruction, which put an
`#NMI` at every instruction boundary. That has two fatal consequences, both
measured: maskable interrupts are never serviced, and compute-bound programs
crawl.

It now free-runs like `PinsZ80::loop()` — `prepareCycle`/`completeCycle` with
`_devs->loop()` each turn — and breakpoints work the z80 way, patching `RST 38H`
into memory. `setBreakInst()` patches one byte: `MemsZ280` is constructed as a
byte memory (`ExtMemory(Endian::ENDIAN_LITTLE)`, no `wordAccess`), so
`put_prog` writes a single byte and the old "a word write would clobber the
neighbour" objection does not apply — the Z-BUS word transfer lives only in
`read_zbus`/`write_zbus`.

Two things had to be got right:

- **The break test must confirm an `RST 38H` actually ran.** A read of `0038H`
  that merely follows a write is not enough: a mode-1 maskable interrupt also
  pushes the PC and vectors there, and so does any stray read of that address.
  Checking `_mems->read_byte(pc) == RST38` at the resume point (`pushed - 1`)
  separates them, and is true for both a patched breakpoint and a program's own
  `rst 38h`. Without it, `mandelbrot` broke out within seconds.
- **The `#AS` wait in `prepareCycle()` must be bounded.** It spins with
  interrupts disabled, so a CPU that stops issuing transactions takes USB with
  it and the board stops answering even the halt port — a reflash is the only
  way back. It now lets interrupts in periodically and gives up if the halt
  switch has been hit.

#### `Devs::vector()` must be overridden per target **[hw]**

`DevsZ280` did not override it, so the base returned 0 and every interrupt
acknowledge handed the CPU `00` -- a NOP instead of the restart the device chose
-- after which nothing serviced the request and it re-acknowledged forever. In
interrupt mode 0 that is fatal; mode 1 does not read a vector and so survived
it. Compare `DevsZ80::vector()`, which forwards to `_usart->vector()`.

#### A prefetch can sit between a push and the vector fetch **[hw]**

`s->prev()` is the pushing transaction only when nothing intervenes, and a
prefetch may. Whether it does is timing-dependent, which makes the failure look
nondeterministic: `echo` stopped on its `rst 38h` every time while `arith`,
whose exit is the same three instructions, ran away -- its stack filling with
the `0039` an `RST 38H` at `0038H` pushes, until the recursion smashed enough
memory to escape into garbage. Scan back a few entries for the most recent
write instead of taking `prev()`.

#### `#NMI` generates no acknowledge transaction **[hw]**

`ST_NMIA` exists in the Z-BUS status encoding, so anchoring `suspend()` on the
acknowledge instead of inferring the push from its position looks like the
obvious improvement. It is not available: **the CPU never puts an acknowledge
cycle on the bus for `#NMI`.**

Measured with ST1-ST3 probed, over the whole `#NMI` low window (6.9us) while
`mandelbrot` ran:

```
   -0.280  MREQ R  #DS yes
   +1.032  MREQ R  #DS yes
   +2.184  MREQ R  #DS yes
   +5.264  MREQ W  #DS yes      <- the PC push
   +6.656  MREQ R  #DS yes      <- the 0066H vector fetch
histogram over 3973 cycles: MREQ 3966, IORQ 5, INTAA 2
```

Nothing but ordinary memory cycles. The two acknowledge-range cycles are at
-853us and -596us, far outside the window -- they are the USART's maskable
interrupt acknowledges. The firmware's own view agrees: during a halt it latches
`Mr Mr ... Mw Mr` and never an `N`.

ST0 showed no transitions in that capture, which is not a probing fault: the
only statuses present were `MREQ` (`0x8`), `IORQ` (`0x2`) and `INTAA` (`0x4`),
and none of them sets ST0. All four lines are known good, confirmed by
executing a `TSET`:

```
   +3.312us  ST=0xF LOCK R  #DS yes  AD0-3=0x0      (the locked access to 2000H)
   ST edges in that capture: ST0 2, ST1 4, ST2 2, ST3 2
```

`ST_LOCK` is `0xF`, every status bit set at once, so one `TSET` proves the whole
status bus in a single cycle -- a cheaper wiring check than reasoning about
which bits a given workload happens to exercise. An `NMIA` (`0x5`) would
therefore have decoded correctly in the capture above had one occurred.

So the NMI sequence is: the instruction being executed finishes, the PC is
pushed to `SP-2`, and `0066H` is fetched -- every one a normal `#DS` memory
cycle. `Signals::nmiAck()` can therefore never be true, and inferring the push
from its position relative to the vector fetch is not a workaround for a missing
feature, it is the only method available. See the prefetch note above for why
that inference must allow exactly one intervening read and no more.

**A correction worth recording.** An earlier capture, on a different Z280 part
and with two status bits unprobed, showed a single `#AS` pulse with `#DS` high
next to the `#NMI` pulse, and this document previously concluded from it that the
acknowledge existed but was being swallowed by the `#DS` wait loops. Re-measured
with the status lines actually connected, no such cycle exists: every cycle in
the window strobes `#DS`. The lesson is the one already in the General section --
a reading taken with probes missing on the very signals that carry the answer is
not evidence.

#### Only an I/O transaction may reach a device **[hw]**

Device selection used to test the address alone:

```c
} else if (_devs->isSelected(ioaddr) && s->readMemory()) {
```

Nothing there required the cycle to *be* an I/O request, so any transaction
whose stale address lines happened to fall in a device's range got an answer —
including an interrupt acknowledge, which carries no meaningful address. Every
device branch is now gated on `s->ioReq()`.

`intAck()` had the same shape of bug: it was written as
`status >= ST_INTAA && status <= ST_INTAC`, and `ST_NMIA` (`0x5`) sits *inside*
that range, so the debugger answered an NMI acknowledge with the USART's vector.
`#NMI` vectors to `0066H` by itself and asks for no vector at all. Name the three
maskable codes explicitly instead. **[hw]**

#### Three ways `suspend()` lost the CPU **[hw]**

All three showed up as "halt, then continue, sometimes does nothing", and all
three are worth knowing because each looks like a different bug:

- **The push is not always `s->prev()`.** A prefetch can land between the push
  and the vector fetch (see above), and insisting on `prev()` made the
  acknowledge go unrecognised whenever one did. Scanning *further* back is
  worse, not better: with four entries a write-heavy program matches an ordinary
  program write and reports a stored datum as the pushed PC. Accept the write
  immediately before the fetch, or one read behind it, and nothing looser.
- **A failed `suspend()` used to be destructive.** `loop()` called
  `_regs->save()` unconditionally, so on failure the save ran with the CPU still
  inside the NMI service and read garbage for *every* register — `SP` went
  `0FFA` → `2408` alongside `PC=32BA`. `restore()` then wrote that back on the
  next continue and destroyed the program, which is why a single failure
  poisoned every iteration after it. Save only on success, as `step()` already
  did.
- **Continuing from a breakpoint goes through the stepper.** `Debugger::go()`
  must single-step *over* a breakpoint at the current PC before running, so
  every continue depends on `suspend()`. A breakpoint that is hit twice and then
  never again is not a breakpoint bug at all.

#### No critical sections are needed on this bus **[hw]**

`prepareCycle()` used to sample and capture under `noInterrupts()`. It does not
need to: #XTALI is driven from that code, so everything the CPU puts on the bus
holds until the next edge produced there. An interrupt can only push a sample
*later* than the propagation delay, never earlier. Removing the guards also
means a CPU that stops issuing transactions can no longer take USB -- and with
it the halt port -- down with it, which previously required a reflash.

The halt/refresh skip loop went with them: refresh is no longer recorded in the
ring, so `s->prev()` stays the previous *program* transaction even when a
refresh reaches the caller, and `completeCycle()` clocks it out on its own.

#### The interrupt acknowledge vector rides AD0-7 **[hw]**

`completeCycle()` drove `swapBytes(_devs->vector())` on an acknowledge, putting
the vector on AD8-15. It belongs on AD0-7 like any byte transfer; the CPU was
reading `00` — a NOP — instead of the restart the device chose. Mode 0 needs
this, since the device supplies a `Call`/`Restart` *opcode* there. Note how the
samples build that opcode:

```asm
db  3EH         ; "LD A," opcode
rst 28H         ; assembles to EF -- becomes the immediate
```

so `A = 0EFH`, the `RST 28H` opcode, which is what gets written to the UART's
vector register.

#### Maskable interrupts were starved by NMI stepping **[hw]**

`PinsZ280::loop()` single-steps the CPU with an `#NMI` at every instruction
boundary, because z280 cannot patch a break opcode into memory (a word write
would clobber the adjacent byte). `#NMI` outranks the maskable inputs, so the
boundary where an interrupt would be taken is exactly the boundary where the
stepper asserts `#NMI`.

Measured with `echoir.hex`: the debugger asserts the interrupt correctly
(`assertInt` fires, the emulated i8251 reports `INT: Rx=38`), the program
reaches `ei`, and a breakpoint on the ISR is never hit — the handler simply
never runs. Same for `echoitr.hex`.

The window is especially tight in the usual Z80 idiom, `DI / CALL / EI / JR`,
because `EI` on the Z280 also disables interrupts "during this instruction and
the following instruction", leaving exactly one boundary per loop where an
interrupt can land.

Contrast `PinsZ80::loop()`, which lets the CPU free-run and watches the bus for
the patched break opcode, so interrupts are serviced normally. Restoring
interrupt support on z280 means giving up per-instruction stepping in `run()`,
which is the same thing the trap route below buys.

#### Per-instruction stepping is too slow for real workloads **[hw]**

Every step costs a full `#NMI` round trip — assert, vector fetch, injected
`RETN`, stack pop, re-park — so a compute-bound program crawls. `mandelbrot.hex`
runs correctly (the PC and SP advance, and it reaches its output formatting
code) but produced no completed output in five minutes. `echo` and `arith` are
fine because they are I/O-bound or short.

#### Injection versus the instruction cache — why traps are the right long-term answer

The whole injection mechanism assumes the debugger sees every fetch on the bus.
**It does not, once the instruction cache is enabled** — a cached fetch never
reaches the bus, so there is nothing to answer. That is why `disableCache()`
exists and why it has to run before anything else at reset.

This is not only a stepping problem. `save()` and `restore()` are injection too,
so with the cache on they are equally blind: whether a sequence gets delivered
depends on whether the CPU happens to have those addresses cached from an
earlier pass.

If the target is ever to run with its cache enabled — which is the realistic
configuration, and the whole point of the part — the debugger has to stop
depending on seeing fetches and use the CPU's own trap machinery instead:

- **Single-Step trap** for stepping (MSR bit 8), vector at IVT offset `3Ch`.
- **Breakpoint-on-Halt trap** for breakpoints — substitute a `HALT` opcode for
  the first byte of the instruction to break on; enabled in the Trap Control
  register (`10`), vector at IVT offset `40h`. The saved PC is the address of
  the trapping instruction, not the next one.

Both need the Interrupt/Trap Vector Table resident and the pointer register set
up (below), and both need the patched/handler memory kept coherent with the
cache — `PCACHE` after any patch.

#### Single-Step trap — investigated, not adopted (yet)

The Z280 has real single-step hardware. Findings, for the record:

- **MSR bit 8 = SS** (enable), **bit 9 = SSP** (pending). The trap fires when
  SSP is set. At the start of each instruction SSP is checked, then SS is copied
  into SSP and the instruction runs — so setting SS gives a one-instruction
  delay before the first trap. **[doc]**
- Simplest way in is `LDCTL` on the MSR with the desired SS/SSP combination. The
  manual lists three others (PUSH PC + PUSH MSR + `RETIL`; a System Call with a
  reserved identifier; Breakpoint-on-Halt). **[doc]**
- The trap pushes **PC and MSR** (in that order). For Single-Step the saved PC
  is the address of the *next* instruction — the manual explicitly contrasts
  this with Division Exception, Access Violation, Privileged Instruction and
  Breakpoint-on-Halt, which save the trapping instruction's own address. **[doc]**
- Return is `RETIL`, which pops a 4-byte program status (PC + MSR). **[doc]**
- Traps that will be re-executed (privileged, divide, page fault) auto-clear SSP
  in the pushed MSR, so only one single-step trap occurs for them. **[doc]**

The catch is vectoring. **All** trap processing uses mode-3-style vectoring from
the Interrupt/Trap Vector Table regardless of the current interrupt mode, so the
table must be resident in memory and the Interrupt/Trap Vector Table Pointer
(control register `06`) initialised before any instruction that could trap. The
table must start on a **4K byte boundary** in physical memory — the pointer
holds only the top 12 bits of the 24-bit physical address. The Single-Step entry
is at offset **`3Ch`** (an MSR word followed by a PC word). **[doc]**

Trade-off against NMI:

| | NMI | Single-Step trap |
|---|---|---|
| Setup in target memory | none — fixed `0066H`, handler injected | 4K-aligned IVT + pointer register |
| Saved status | PC only (1 word) | PC + MSR (2 words) |
| Return | `RETN` | `RETIL` |
| Timing sensitivity | must assert after the opcode fetch | none — fires by design |
| Block instructions | can interrupt mid-block; saved PC is the block instruction itself | steps over the whole block |

NMI is cheaper for a debugger that does not want to reserve 4K of the target's
physical memory, and it is what is implemented today. But NMI-based stepping
only works because the cache is off, and the cache is off only because
injection needs the bus. The moment the cache is to be enabled, the trap route
is not an optimisation — it is the only thing that works.

#### Sample status

Checked against `samples/z280/` on hardware:

| sample | result |
|---|---|
| `echo.asm` | works — characters echoed, LF added after CR, clean exit |
| `arith.asm` | works — all 30 results correct for signed 16-bit |
| `echoir.asm` | works — interrupt mode 1 |
| `echoitr.asm` | works — interrupt mode 0, vectored restarts |
| `mandelbrot.asm` | works — 4.74s per frame, loops until stopped |

All five behave correctly; the four that exit do so at their own
`rst 38h`. `mandelbrot` loops until stopped -- Ctrl-Space on the console
or any byte on the halt port.

Mandelbrot only became usable once #XTALI was sped up: at the original
100ns phases it never reached the end of a frame. The larger win was free-running
`loop()` rather than stepping it (below).

Do not take a short `mandelbrot` run as a frame time. It loops until stopped, so
a run that ends on its own has exited early: runs measured at "131 lines in 64ms"
were doing that. A frame is **4.74s**, measured between the blank lines the
sample prints between iterations — three consecutive frames came out at 4.74s
each. A run that stops by itself is a *failure* signal for this sample, not a
timing result.

Those blank lines are the right way to time any redrawing sample, and the only
way to compare a slow target with a fast one: `bionic-control.py run <cap> <n>`
counts them, stops after n iterations and prints each interval. A fixed duration
cannot serve both ends of the range -- `arith` finishes in 0.2s where a frame of
`mandelbrot` takes 4.74s, and a slower target takes minutes.

The samples exit with the shared Z80 convention: write `RST 38H` (`FFH`) into
the restart vector and restart to it. z280 now honours that in `loop()`, gated
on the vector actually holding `FFH` — necessary because `ORG_INT` and
`ORG_RST38` are both `0038H`, so during normal running the vector holds the
`JP` to the ISR and must not be mistaken for an exit.

### Z80 / Z180

Byte bus, one byte per read, sequential-cursor injection — so the faked-POP
idiom (`POP rr` followed inline by its payload) works and is used throughout
`regs_z80.cpp`. Do not copy it to a word-bus target. **[code]**

`z180`/`z80` `completeCycle()` treats an I/O address as 8-bit and ignores the
high 8 bits, so bugs in the high byte of an I/O address are not observable
there. **[code]**

### TMS9900 family

`pins_tms9900_base.cpp::suspend()` is the reference for the "assert the
interrupt only after the opcode fetch" idiom: it walks the loop with a
`bool assert_nmi` latch and asserts on the first cycle where `s->fetch()` is
true. Targets without an `#M1`-equivalent have to substitute "the transaction
`resumeCycle()` handed back", which is the opcode fetch by construction — every
sequence parks on the read that follows it. **[code]**

### 6502

`RegsMos6502` takes its `Mems*` as a constructor argument rather than reaching
through `Pins`, because `Pins::_mems` is protected and only `Target` is a
friend. Follow that pattern when a `Regs` implementation needs memory access —
and note `Pins` builds `_regs` before `_mems` in some targets, so the order has
to be swapped first. **[code]**

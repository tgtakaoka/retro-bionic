# Development style

Hard-won notes from bringing up targets in this codebase. Things that cost real
debugging time and are not obvious from the source.

Each claim is marked **[hw]** when it was verified on the bench, **[doc]** when it
comes from a datasheet or manual, and **[code]** when it is a property of this
codebase.

The General part holds what applies to every target. Each section under
Per architecture opens with that chip's pitfalls; read them before touching
its code.

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

The idioms differ too. A faked `POP` — the opcode followed inline by its
payload — works only under a cursor (the Z80's); under address-keyed injection
the stack read lands at SP, nowhere near the window, and is answered from real
memory. **[hw]** Prefer real load instructions there; where a register has no
load (a flags register, typically), stage the value in memory and point SP at
it.

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
recovered from the bus trace. The same prefetch means a sequence "exits" before
its last instruction has run: whatever the host does next that depends on that
instruction must wait for it (the Z280 ends such sequences with a taken jump).
**[hw]**

### Pipeline flushes force re-fetches

Any instruction that flushes the prefetch queue makes the CPU re-fetch addresses
it has already passed. So does interrupt and trap processing. An injected window
must stay answered across those re-fetches, not just until its last byte has
gone out — a re-fetch answered from memory is garbage the CPU then executes.
The same applies to an injected interrupt handler's vector: serve it for as
long as the CPU asks for it. **[hw]**

### Resume from an explicit origin, never from the ring **[code]**

A CPU parked mid-transaction has to be resumed at the address it was parked
at. That address cannot always be re-read from the pins (a multiplexed bus
carries data by then), and it cannot live in the ring: `Cycles::reset()` and
`Cycles::discard()` both clear the head slot, and a dump or an exit is exactly
when they run. The i8080/i8085 shape is the right one: `resumeCycle(addr)`
takes the address, the caller keeps it, and `_regs->nextIp()` is where the CPU
is parked between operations. (An early Z280 resumed from the ring; see its
section.)

**The ring holds one fewer cycle than it has slots.** The head slot is the
transaction in progress. `Cycles::next()` used to let the count reach
`MAX_CYCLES` before moving the tail, so a dump of exactly that many cycles had
tail and head on the same slot and printed nothing. **[code]**

**A run's dump shows the program, not the debugger.** The break unwinding, the
halt's interrupt push and vector fetch, and every injected read and captured
write of `_regs->save()` must stay out of it: discard what an exit injected,
and either dump before saving (as `step()` and the Z80 do) or hold the ring
while saving (the Z280). **[code]**

### Keep bus-keepalive cycles out of the ring **[code]**

Refresh transactions are the bus keeping DRAM alive, not the program doing
anything, and at debugger clock speeds one can land between almost every pair
of real transactions. Recording them:

- breaks `s->prev()`, which callers use to mean "the previous *program*
  transaction" — e.g. matching an interrupt vector fetch against the PC push
  that must immediately precede it;
- floods the 128-entry ring so a dump shows nothing else.

`completeCycle()` should not advance the ring for them. That, not disabling
refresh at the source, is the fix (the Z280's refresh cannot be disabled).

### Bound every wait loop **[hw]**

`loop()` only polls the halt switch *between* steps. A wait loop that never
returns cannot be broken into from the halt port. Give every "wait for the CPU
to do X" loop a guard and a failure path. This bit three times: an unbounded
refresh-skip in the Z280's `prepareCycle()`, its NMI-acknowledge wait in
`suspend()`, and the MC6800 family's wait for a context push from a CPU in
`WAI`.

The RTWDOG backs this up: the prompt, every completed bus cycle and every
halt switch poll feed it, so a loop that does none of them reboots the Teensy
after 4 s, and the banner reports the watchdog reset. A reboot still loses
breakpoints and the target's state, and a loop that keeps completing bus
cycles is not caught at all, so bound the loop anyway. The profile image's `X`
command hangs on purpose, to test it. **[code]**

### The halt port **[hw]**

The Teensy builds with `USB_DUAL_SERIAL`. `/dev/ttyACM0` is the debugger
console; **any byte written to `/dev/ttyACM1` aborts a running CPU**
(`serialEventUSB1()` → `Pins::isrHaltSwitch()` → `_halted`, polled by `loop()`).
It is the only way to stop a run that does not end on its own — provided the
wait loops are bounded.

**It is serviced only from `yield()`.** **[code]** A run reaches `yield()`
only through console I/O, so a program that never enabled the console device
could not be halted: the CLI never came back, which looked exactly like a
wedged board (found with `samples/z280/mmu`, the first sample with no USART
setup). `Pins::haltSwitch()` now calls `yield()` itself. **[hw]**

**The flag used to re-arm itself.** `serialEventUSB1()` raised `_halted` but
never *read* the byte, and the core re-calls the handler from every `yield()`
while the port still holds data, so **one abort also killed the following
run**: every `G` returned at once with a register dump. Fixed by draining in
the handler (PR #38). **[hw]**

**An abort leaves the emulated USART repeating.** After a run is stopped
mid-cycle, the USART keeps re-delivering its last received character, which
floods `ttyACM0` and buries the CLI prompt. A harness must drain the console
after any abort, and should prefer to stop an interactive sample the way the
sample itself expects (NUL for `samples/z280`) over aborting it. **[hw]**

A working recovery order for an unresponsive board, in increasing violence:
Ctrl-C (`0x03`), Ctrl-Space (`0x00`), then the halt port. If none of the three
draws a reply, the firmware is stuck in a wait: the watchdog reboots it within
4 s if the wait feeds nothing, and otherwise only a reflash brings it back.

### Compare a pin read against `LOW`, never `HIGH` **[code]**

`digitalReadFast()` is not guaranteed to normalise to 1 — a fast read can hand
back the masked register bit, `1 << bit`. That is truthy but **not equal to
`HIGH`**, so `read() == HIGH` can be false while the pin is high, and the bug
appears only on the pins whose bit position is not 0. `LOW` is 0, so `== LOW`
and `!= LOW` are exact whatever the accessor returns. Write the test that way
even when the current expansion happens to be safe.

### Commented-out debug pin calls still count as time **[hw]**

`assert_debug()` and `negate_debug()` cost about 10 ns of pin writes each,
and bus code written while they were live took that time as margin. The
normal build comments them out, so put the time back where it mattered:
a `delayNanoseconds()` of one call's worth after a bus drive, for setup
before the clock edge that latches it, and before a sample, for the
signal to settle. A delay on the other side only stretches the cycle.
(The TMS320C15 drew wrong pixels until it got them.)

### A logic analyser on the bus can break the board **[hw]**

Probe leads add enough capacitance to stop a marginal target working at all:
sixteen leads on the Z280's multiplexed bus took a commit that had just passed
all five `samples/z280` programs to *zero* passes, every time, with no change
to the source. Unplugging them restored it.

- **Bisect against a known-good commit before believing any code theory.** With
  the probes on, every build failed, so each change tested in isolation looked
  like the culprit. Checking out the last commit known to pass ends it at once;
  `git reflog --date=iso` dates the builds, so the timestamps of a session's
  passing run identify which commit to try.
- **Timing measured with the probes on is not the unloaded timing.** Absolute
  setup and hold figures taken that way describe a bus that no longer behaves
  like the one shipping.
- **A reading taken with probes missing on the signals that carry the answer
  is not evidence** (the Z280's phantom NMI acknowledge).

### Bus level holders make the data bus turnaround uncritical **[hw]**

The port 6 pins **P6.16 to P6.31** — the data bus, among other uses — each
carry a **bus level holder**. When the controller switches one of those pins
from output to input, the level it was driving is *held* on the net until the
CPU drives the pin itself. So the data bus never floats across the turnaround:
a CPU still sampling between `inputMode()` and taking the bus over reads the
level the controller last drove.

- Releasing the bus does not have to be timed precisely against the point where
  the CPU stops sampling. Effort spent shaving that margin is wasted.
- A read of a pin in that range tells you what is *held* there, which is not
  evidence that the CPU is driving it. Do not infer bus ownership from a level.

### Verify opcodes with the assembler, not by hand

libasm ships a CLI assembler (`asm`, built from libasm's `cli/`); use it rather
than hand-encoding injected sequences, especially for relative jump
displacements:

```
printf '        CPU Z280\n        ORG 0\n        JR 0000H\n' > x.asm
asm -C z280 -l /dev/stdout x.asm -o /dev/null
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
`pdftotext -layout` on the PDF makes it greppable. A measured behaviour that
contradicts an assumption is usually documented somewhere, and a "this should
work" fix that does not work usually has a paragraph explaining why.

### Check cycle tables against the chip, not only the manual **[hw]**

The halt and breakpoint logic of a target that follows bus-cycle sequences
is only as right as its tables, and the manuals they come from can be wrong
or ambiguous (the MC68HC08's AN2627 is one example). `scripts/record-cycles.py`
runs one instruction at a time on the profile image (`-D PROFILE_CYCLES`, the
`teensy41-profile` environment, one build for every target), in memory filled
with the target's trap so any transfer stops at once, and compares what the
chip did with the tables. Everything specific to a target lives in a plugin
beside it, `debugger/<arch>/tools/cycles_<chip>.py`, named on the command line
with the recording and the channels file. The profile loop prints every cycle
without consulting the tables; where a matcher segments cycles into
instructions, it also prints the matcher's verdict, so one run checks the
table, the matcher and the chip together.

The profile loop holds the debug pin active for the run, so `--capture` can
check the firmware's view of each cycle against a logic analyzer triggered
on it. Decode strobes with a minimum width and merge a strobe split by a
nanosecond blip: crosstalk produces both, and either one reads as an extra
cycle. The pin drops as the trap's vector read ends, too close to tell
which side of it that cycle falls, so leave it out of the comparison.

Sample each signal where it is stable, which is not always where the data
is (see the HD6309's R/`#W`). Compare only the kind of a dummy cycle:
nothing drives the bus then, and the bus holders show whatever was there
before. A flash clears the emulated memory, and the trap fill with it, so
fill again before the next run.

Fix a table in its `.txt`, never in the `.cpp`: `inst_<chip>.awk` turns
the `.txt` into the matcher's arrays, and puts the command that did it on
a `// Generated by:` line before them. Run that command in the arch's
directory and replace what follows the line. The awk numbers the
sequences in the order the `.txt` first uses them, so a fixed row can
renumber the rest; there is no list in the awk to edit. `-v MODE=pretty`
prints the `.txt` back aligned instead. **[code]**

### A frame that completes is not a frame that is right **[hw]**

`frames = 1` only proves a mandelbrot frame finished. Checking the output
against `samples/arith/mandelbrot.golden` found three bugs that had passed that
test for as long as it existed: the 8080's memory corruption, a wrong formula
in the INS8070 sample, and division bugs in several `arith.inc` files. A
mirror-symmetric but wrong frame points at a sample's arithmetic; scattered
wrong characters and restarts point at the bus.

The `[arith]` expect block is matched as one substring, so every line has to be
what that sample actually prints, in order. Lists copied from another target
were wrong for most targets; derive them from the sample's own `arith.asm`.

### Bound a mandelbrot run by time, not by frames **[hw]**

One frame takes 5 seconds on a Z280 and 14 minutes on an F3850, so a regress
run waiting for `frames = 1` cost minutes per slow target. `lines = 6` halts at
the first completed frame, or once 6 rows are drawn and 20 seconds have
passed, and checks everything drawn against the golden frame. A fast CPU
finishes a frame, a mid-speed one runs 20 seconds, and a slow one stops at 6
rows.

`[gountil]` targets `loop_y` rather than `loop_x` on mid-speed CPUs (6 rows
within 20 seconds), so each go runs a whole row instead of one pixel.

To time a redrawing sample by hand, count the blank lines it prints between
frames: `bionic-control.py run <cap> <n>` stops after n of them and prints each
interval. A run that ends on its own has exited early, and is a failure, not a
timing. **[hw]**

### Samples on another console device **[code]**

`io = "SCI"` in a sample's table routes the console through that device with
the `I` command for that sample only. Reset does not change the selection, so
the script reads the enabled device from `I`'s listing first and switches back
to it afterwards, pass or fail.

### Reset after uploading, not only before **[hw]**

`R` before an upload stops the CPU, but it resets from the vector already in
memory -- the *previous* program's. Uploading does not touch the PC. When
every sample starts at the same address this never shows; on the MC6800,
`mc68xx` starts at 1000 while the others start at 0100, so `G` ran the stale
program. `bionic-regress.py`'s `load()` resets again after the upload.

For a CPU that fetches its PC from a reset vector, `[reset]` names the
vector's bytes, most significant first -- `PC = "[FFFE FFFF]"`, or
`"[FFFD FFFC]"` on the 6502 -- and the reset case checks the PC against the
value the loaded sample puts there.

### Some CPUs report the PC one byte early **[hw]**

The SC/MP (INS8060) and INS8070 increment PC *before* each fetch, so the PC
register holds the address before the next instruction: a stop at `A` prints
`PC=A-1`. Breakpoints and go-until targets still take `A`; only the printed PC
is offset. `pc_offset = -1` in a regress file's `[breakpoint]`/`[gountil]`
tells the script so. To start execution at `A` by hand, set `PC=A-1`.

### Checking the analyser leads and board files **[hw]**

A preset's labels are only as good as the leads. Decode a reset capture before
trusting a new channel set: the debugger's register-save sequence after reset
injects known instruction bytes on reads and writes known register values, so
swapped strobes show at once (INS8070: `#RDS`/`#WDS` crossed on the probe).

The `schematics/<board>/*_bionic.toml` pin maps are hand-written; check them
against the PCB's pad nets, not the other way round. Found wrong: the
INS8060's data pins reversed, P0x instead of P1x on the MC6800 and MC6809E,
and a duplicated pin on the HD6301.

Keep captures triggered and short. `trimDataSeconds` only trims after the
trigger fires, so a stopped, untriggered capture holds the whole run; exporting
one wrote gigabytes and filled the disk, and closing the capture did not stop
the export.

### Order bench work by image, not by fix **[hw]**

Seating a chip takes moments; flashing the Teensy takes minutes and wipes
the emulated memory. Batch everything one image can test before flashing
the next, and when a fix needs a new image, first finish what the image
already on the board can still do: a TMS7000 fix found mid-run waited
while the P8095BH and TMS320C15 checks ran on the same profile image.

### `ENABLE_SERIAL_HANDLER` builds were broken in many targets **[code]**

The option is off by default, so nothing noticed that most `devs_*` files
declare a `SerialHandler` under it without including `serial_handler.h` or the
handler's own header. Syntax-check a target with and without
`-DENABLE_SERIAL_HANDLER` when touching its devices.

## Per architecture

### Z280

**Pitfalls.**

- **Byte lanes follow the absolute address**, and a word read at an odd
  address is the aligned pair; getting either wrong shifts injection by a
  byte or poisons the instruction cache.
- **On-chip memory is a cache at reset**: a cached fetch never reaches the
  bus, so injection is blind until `disableCache()` has run.
- **Refresh cannot be turned off**, only kept out of the ring; its rate field
  must still be `3F`, and whatever sets it must put the I/O Page back to 0.
- **`#NMI` must go out during the opcode fetch** to step exactly one
  instruction, and in modes 0-2 it makes no acknowledge cycle; the push is
  inferred from its position, with at most one prefetch in between.
- **Sample the address inside the `#AS`-low window**, the status after the
  rise; `completeCycle()` cannot count T states.
- **The PC is logical, the bus physical**: a resume origin is a bus address,
  and only the parked page's frame is known.
- **A captured sequence ends with a taken jump**, or the host acts before
  its last write.
- **Analyser leads on the multiplexed bus can stop it working at all**:
  hours went into a `Cycles::reset()` change, a reset setup delay, a
  `clk_delay_ns` margin and an `#AS` polling rewrite while the probes were the
  fault. **[hw]**

#### Bus, lanes and sampling

**Z-BUS byte lanes.** The even-address byte rides AD8-15 and the odd-address
byte AD0-7 (§13.5.1.1). The lane is chosen by the **absolute address**, not by
an offset into a buffer; the two agree only at an even origin, and at an odd
one the whole stream is delivered shifted by one byte. This applies equally to
injection and to `MemsZ280::read_zbus`/`write_zbus`. **[hw]**

Uncached, the CPU issues a word read at every PC value (instruction
boundaries), not one per two bytes, so it asks on both parities. **[hw]**

**A word read at an odd address is the aligned pair.** Whatever A0 says, the
CPU treats the two lanes as bytes `addr & ~1` and `addr | 1`, and with the
instruction cache on it files both in the line. Answering an odd fetch with
`[addr]` and `[addr+1]` looked right for years, because uncached the CPU takes
only the lane its parity picks -- but a jump to an odd target then cached
`[addr+1]` as the byte *before* the target, usually the `RET` of the routine
above, which later ran as the entry's second byte: `queue_remove` returned into
the receive ISR's prologue. That is why interrupt-driven samples failed with
the instruction cache on, and only at even link offsets. **[hw]**

**Sample the address inside the `#AS`-low window, the status after the rise.**
AD0-15 carries the address only while `#AS` is low; the rise latches it and the
CPU then turns AD around for data. `prepareCycle()` sampled both 60 ns *past*
the rise, so `getAddr()` could read whatever AD had become, and a stopped run
gave a PC outside the program (`A409`, `EE53`) with a different byte count
every time. Sampling the address at the end of the `#AS`-low window made runs
identical: `PC=021F`, 2232 bytes, 8/8. `#AS` cannot rise until #XTALI is driven
low, so waiting there costs nothing. `ST0-3`, `R/#W`, `B/#W` and `#AS` are
3-state and valid only from the `#AS` rise, so the two samples straddle it:
**[hw]**

```
while (signal_as() != LOW) { ...clock... }
delayNanoseconds(addr_delay_ns);   // settle, still inside the #AS-low window
s->getAddr();
xtali_lo();                        // ...which makes the #AS rise
delayNanoseconds(status_delay_ns);
s->getControl();
```

**`completeCycle()` cannot count T states.** It is sometimes entered straight
after `resumeCycle()`, resuming out of a `#WAIT` stretch, with no way to know
which T state the CPU is in. So both edges have to be found by watching `#DS`.
Latching writes "one cycle past the T2 `#DS` fall, at the T3 rise" — where the
data really is valid — broke every run for this reason. Getting the write
sample later without tracking state through `resumeCycle()` remains open.
**[hw]**

**No critical sections are needed.** `prepareCycle()` used to sample and
capture under `noInterrupts()`. #XTALI is driven from that code, so everything
the CPU puts on the bus holds until the next edge produced there; an interrupt
can only push a sample later. Without the guards a CPU that stops issuing
transactions no longer takes USB, and the halt port, down with it. The `#AS`
wait has no guard of its own, so such a CPU is caught by the watchdog.
**[hw][code]**

#### Clocking

`CLK = XTALI` with CS=01 in the Bus Timing and Initialization register (CS=00
gives XTALI/2), ~16 ns XTALI→CLK propagation. On a memory transaction `#AS` is
asserted for one XTALI phase; `#DS` falls one phase later on a read and two on
a write, staying low three phases and two respectively. **[hw]**

**The CS latch does not take effect until the first bus transaction.** While
`#RESET` is asserted, and on past it, `CLK` stays on a fixed reset divider.
Checked in the `#WAIT` hold window after `#RESET`, `CLK` changes 7 times in 28
samples whether CS was latched `01` or `00`, so `resetPins()`'s "steady" check
ran all its retries every time. Past `prepareCycle()` it is a clean
discriminator: sampling `CLK` once per `#XTALI` cycle gives **0 changes in 16 at
CS=01 and 12 at CS=00**. It is safe to spend cycles there because `#WAIT` parks
the CPU in T2; before, extra cycles carry the CPU past T1 and leave
`prepareCycle()` waiting on an `#AS` that has gone. **[hw]**

**Sustained clock rate is set by host code, not by the delay constants.** Over
a whole `mandelbrot` run, `CLK` and `#XTALI` both average **3.52 MHz** (323,007
cycles in 91.75 ms), median period 280 ns, max 352 ns, **89% high**. The low
phase is the 20 ns delay plus GPIO overhead; the high phase absorbs whatever
`completeCycle()`, `_devs->loop()` and `prepareCycle()` cost. The ~9 MHz seen
during tuning is the bare toggle rate of the `prepareCycle()` wait loop;
trimming the delay constants further buys very little. **[hw]**

#### Cache, refresh and the I/O page

**On-chip memory is a cache at reset.** Cache Control is control register `12`,
reset value `20` (M/C=0, I=0, D=1) — instruction caching on.

- `I=1` stops *new* lines being filled but does **not** invalidate what is
  already cached; a short loop cached before `I=1` keeps running from cache.
  `PCACHE` (`ED 65`) drops the lines. **[hw]**
- Fixed-address mode (`M/C=1`) also frees the bus but needs all 16 line tags
  initialised by data reads first, which injection cannot carry — there is no
  `#M1` to tell a data read from a fetch. **[hw]**

**Refresh cannot be turned off.** Clearing Refresh Enable does not stop the
transactions; §9.3 makes them a *minimum bus transaction rate*: "if the refresh
timer reaches 0 and no external bus transaction has occurred since the last
time the refresh timer elapsed, then a refresh transaction will be generated."
A parked or slowly-stepped CPU is exactly that condition, and at debugger
speeds the timer has always elapsed whatever the rate. Measured: the
refresh-to-MEM ratio was unchanged. **[hw][doc]**

Still set the register (`E=0`, rate `3F`), as a board with no DRAM should.
**Rate 0 is not the slowest setting** — the field is documented over
`0 < n < 63` (once every 4n clocks) with zero a separate special case — and
keep refresh out of the ring (see General). **[doc]**

**The I/O Page register is cleared to zero by reset.** On-chip peripherals live
on pages `FE`/`FF`, and on-chip I/O makes no external bus transaction. Anything
that changes the page must put it back to 0, or every later `OUT` is handled
on-chip and the debugger waits for a bus cycle that never comes.
**[hw][doc]**

**Control register addresses** (`LDCTL`, address in C) **[doc]**:

| Register | Addr | | Register | Addr |
|---|---|---|---|---|
| Master Status (MSR) | `00` | | Trap Control \* | `10` |
| Bus Timing and Control \* | `02` | | Cache Control \* | `12` |
| Stack Limit | `04` | | Local Address \* | `14` |
| Interrupt/Trap Vector Table Pointer | `06` | | Interrupt Status | `16` |
| I/O Page \* | `08` | | Bus Timing and Initialization \* | `FF` |

\* 8-bit; only the low byte of the source register is written.
`LDCTL HL,(C)` of an 8-bit register leaves H undefined. **[hw]**

#### Injected sequences and the resume origin

**Every sequence threads an `org`.** An early Z280 resumed from the ring, and
every reset of it resumed at address zero, set an injection origin of zero and
corrupted everything downstream; wrapping the two calls to copy the slot out
and back was the next mistake. Now `execute()` resumes at `org`, keys its
window on it, and hands back where it parked — the exit it was told, or
wherever the guard ran out. Sequences chain by handing the same variable
along. **[code]**

**The origin is a bus address, and the PC is not.** The saved PC is 16 bits of
logical address; the bus shows 24 bits of physical address, the MMU's page
being 4K. So `RegsZ280` keeps the frame beside the PC (`park(pc, addr)`),
`nextIp()` puts the two back together, and `physical(logical)` translates any
other address on that page the same way — a one-page MMU emulation. Off the
page identity is assumed, as a disabled MMU does. Exits given to `execute()`
are physical: `execInst(JP_PC, ..., org, physical(pc))`. **[hw]**

**Capture the frame on a memory read.** Right after reset, or after the `RETN`
that ends a step, `prepareCycle()` can return a refresh, whose address is the
refresh counter's. `skipToRead()` clocks past anything that is not a memory
read before the address is kept. **[hw]**

**A captured sequence ends with `JR $+2`.** The prefetch reaches the address
past the window before the last `PUSH` or `LD (HL),A` has written, so on the
straight-line stream the CPU runs on into whatever memory holds. The taken
jump flushes the pipeline and fetches the exit again — after the writes. Any
sequence whose tail the host depends on next (a pending `POP AF`, the NMI frame
before `RETIL`, `restoreRegs`'s pop) needs it. The old firmware hid the miss by
re-latching its origin from the next sequence's first read, and the 128 cycles
of garbage in between made IX, IY, I and USP drift from step to step. **[hw]**

Each sequence declares where it leaves off (`EXIT_END`, `EXIT_ORG`, or its
trailing jump's target); the loop cannot infer it. **[hw]**

#### Running: free-run, breaks and halts

**`loop()` free-runs.** It once called `rawStep()` per instruction, an `#NMI`
at every boundary. `#NMI` outranks the maskable inputs, so with `echoir.hex`
the interrupt was asserted, the program reached `ei`, and the ISR never ran
(`EI` also holds off interrupts for the following instruction, leaving one
boundary per `DI / CALL / EI / JR` loop); and `mandelbrot` produced nothing in
five minutes. It now runs like `PinsZ80::loop()`, `_devs->loop()` each turn,
with breakpoints patched as `RST 38H`. `setBreakInst()` patches one byte:
`MemsZ280` is a byte memory to `Mems`; the Z-BUS word transfer lives only in
`read_zbus`/`write_zbus`. **[hw]**

**A break must prove an `RST 38H` ran.** A mode 1 interrupt also pushes the PC
and vectors to `0038H`. Checking `_mems->read_byte(pc) == RST38` at the resume
point separates them; without it `mandelbrot` broke out within seconds. The
exit convention (`FFH` written into the restart vector) is honoured only while
the vector holds `FFH`: `ORG_INT` and `ORG_RST38` are both `0038H`, so in
normal running it holds the `JP` to the ISR. **[hw]**

**A prefetch can sit between a push and the vector fetch.** Whether it does is
timing-dependent: `echo` stopped on its `rst 38h` every time while `arith`,
same exit, ran away, its stack filling with the `0039` an `RST 38H` at `0038H`
pushes. Scan back for the most recent write instead of taking `prev()`. In
`suspend()` accept the write immediately before the `0066H` fetch or one read
behind it, nothing looser: with four entries a write-heavy program matched an
ordinary write and reported a stored datum as the pushed PC. **[hw]**

**`#NMI` timing.** It must be asserted **during** the opcode fetch of the
instruction to step — after `resumeCycle()` hands that fetch back, before
`completeCycle()` finishes it. Later steps **two** instructions; earlier
vectors without running anything. Cutting the refresh rate removed idle cycles
that had given `#NMI` time, and a step started landing on two; it became
reliable only once `resumeCycle()` was guaranteed to return the opcode fetch
itself (the `JP` at the tail of `LD_ALL` had been parking on a prefetch).
**[hw]** See the TMS9900 for the idiom.

**NMI in modes 0-2** pushes **PC only** and vectors to `0066H`; `RETN` (`ED 45`)
returns. The pushed value is the next instruction's address, except for block
instructions, where it is the block instruction's own. **[hw][doc]**

**There is no acknowledge cycle for `#NMI` in modes 0-2.** With ST0-ST3 probed
over the whole 6.9 us `#NMI` window while `mandelbrot` ran, every cycle was an
ordinary `#DS` memory cycle — reads, the PC push, the `0066H` fetch; the only
acknowledges were the USART's, far outside it. A `TSET` proves the whole status
bus in one cycle (`ST_LOCK` is `0xF`). So `Signals::nmiAck()` is never true
there, and inferring the push from its position is the only method. An earlier
capture with two status bits unprobed had suggested a swallowed acknowledge;
it was wrong. **[hw]**

**Three ways `suspend()` lost the CPU**, all showing as "halt, then continue,
sometimes does nothing": insisting on `prev()` for the push (above); saving
after a failed `suspend()`, which read every register from inside the NMI
service (`SP` `0FFA` → `2408`) and wrote that back on the next continue — save
only on success; and forgetting that every continue from a breakpoint steps
over it first, so a breakpoint hit twice and then never again is a stepper
bug. **[hw]**

**Breakpoints before a `HALT`** are found by reading the opcode from memory, so
a prefetch queue filled before the `HALT` was written can still carry the CPU
into it. `rawStep()` refuses to step a `HALT`, and a run that halts cannot be
saved: there is no boundary to inject at. **[hw][code]**

**`run()` saves before the dump**, holding the ring so the debugger's own
cycles reuse the head slot, because the dump needs the Cache Control the
program stopped with. Each `loop()` exit `Cycles::discard()`s what it injected,
and `loop()` returns whether the registers can be saved at all. **[code]**

#### Interrupts and devices

- **The acknowledge vector rides AD0-7**, like any byte transfer. Driven on
  AD8-15, the CPU read `00`, a NOP, instead of the restart. Mode 0 needs it:
  the device supplies an opcode. The samples build one as `db 3EH` then
  `rst 28H`, so `A = 0EFH` is written to the UART's vector register. **[hw]**
- **`Devs::vector()` must be overridden.** `DevsZ280` did not, so every
  acknowledge got `00` and re-acknowledged for ever; fatal in mode 0, unseen in
  mode 1. Compare `DevsZ80::vector()`. **[hw]**
- **Only an I/O transaction may reach a device.** Selection once tested the
  address alone, so an interrupt acknowledge with stale address lines got a
  device's answer; every branch is gated on `s->ioReq()` now. `intAck()` names
  the three maskable codes: `ST_NMIA` (`0x5`) sits inside
  `ST_INTAA..ST_INTAC`, and answering it with the USART's vector corrupted
  the NMI sequence. **[hw]**

#### MMU and user mode **[hw]**

`samples/z280/mmu_echoir.asm` runs its echo loop in user mode with user page
0 mapped to physical 0A5000H and every other user page invalid; the USART
setup, the receive interrupt handler and putchar/getchar run in system mode,
reached by `SC` traps through the vector table at 2000H (interrupt mode 3,
`RETIL`). What it relies on, and what the debugger cannot do yet:

- The system stack, the vectors (0038H, 0066H, the table) and the page the
  debugger calls into (`CALL 8000H`, page 8) must be valid system pages, and
  page 8 must map to itself: an exit off the parked page is translated by
  identity.
- Breakpoints and the `RST 38H` exit are recognised by reading memory at the
  *logical* PC, so they work only in pages mapped to themselves; a user-mode
  program exits through a system call that breaks in system mode.
- Interrupt mode 3 is required for a user-mode program with interrupts: in
  modes 0-2 an interrupt clears U/S and "the previous condition of the MSR is
  not saved" (6.2.1), so `RETI` cannot return to user mode.
- **A mode 3 NMI** shows on the bus as an acknowledge cycle, then
  `W(PC) W(MSR) W(identifier)` on the system stack, two reads from the vector
  table, and the handler fetch. `suspend()` parks *there*, in system mode,
  whatever mode the program was in; `save()` takes PC and MSR from the frame,
  and `restore()` writes them back and returns with `LD SP,msr_slot; RETIL`.
  The handler never executes; its table entry must supply an MSR that
  disables interrupts, and the system map is assumed to be identity.
- **The MSR and the I/O Page register are registers like any other**: the
  third dump line shows `USP=` under `SP=`, then `MSR=` and `IOP=`, and
  `=MSR` / `=IOP` set them. They are read and written with `LDCTL` (C=00H,
  08H); the MSR goes last, just before the jump, since it takes effect at
  once. Inside the mode 3 NMI service the MSR is the frame's word and `RETIL`
  restores it.
- Code at a 24-bit physical address is assembled with a 24-bit `org`; the
  assembler emits extended-linear HEX records, which the loader honours and
  forgets again when the upload ends.

Two things that cost an afternoon: `ld HL, (IVT>>12)<<4` assembles as an
*indirect* load -- a leading parenthesis is an address, whatever follows it;
and writing 0 to the emulated USART's vector register disables its interrupt,
even though in mode 3 the byte is only the identifier pushed. **[hw]**

#### Programs with the cache enabled **[hw]**

`samples/z280/cmandel.asm` (via `enable_cache.inc`) caches instructions and
data; the debugger caches nothing after reset, so a program turns the cache on
itself. What that costs the debugger:

- A cached fetch never reaches the bus, so nothing may be injected where the
  program's lines are. Every stop therefore parks at the *vector* fetch
  (`RST 38H`, 0066H or the mode 3 handler), which the program does not
  execute, and the sequences run from there in system mode.
- A miss fetches one word (per-word valid bits, burst off), so an injected
  sequence is fetched by even words; a jump target is matched by its word,
  not its byte address, and the CPU resumes at the word the bus showed.
- The entry sequence purges, keeps HL and BC, reads Cache Control and sets
  60H, all in one window, before anything else; the return sequence runs
  from 8000H, sets the program's Cache Control back, reloads BC and HL,
  purges, sets SP and returns -- what is fetched after the purge stays
  cached, so it must not be the vector's line. A step leaves the cache off
  (one instruction) and the next save keeps the recorded value.
- The transaction a halt is noticed on can be an acknowledge cycle with no
  address; the program's page frame comes from its last memory read.
- The third dump line ends in `CACHE=` -- `__`, `I_`, `_D` or `ID` for what
  the program caches -- and `=IC 1` / `=DC 1` (0 to turn off) edit it, so a
  plain `mandelbrot.hex` can be run cached without touching its source. The
  restore sequence writes the program's MSR after Cache Control (not after a
  mode 3 NMI, where `RETIL` restores it). After an NMI in modes 0-2 the
  interrupt enables sit in the Interrupt Shadow register, which only `RETN`
  reads back: the dump shows them as `??` and `=MSR` cannot set them.
- The status lines cannot tell whether the cache was on: ST `1000`
  (cacheable) is the MMU page's attribute, carried by every memory cycle with
  the cache off too (12725 of 12725 recorded reads). With the instruction
  cache on, the dump stays raw: the fetches never reach the bus.

#### Single-Step and Breakpoint-on-Halt traps — investigated, not adopted

The CPU's own trap machinery would step and break without seeing fetches.
Findings, for the record: **[doc]**

- **MSR bit 8 = SS** (enable), **bit 9 = SSP** (pending). The trap fires when
  SSP is set; at the start of each instruction SSP is checked, then SS is
  copied into SSP, so setting SS gives a one-instruction delay. The simplest
  way in is `LDCTL` on the MSR (the manual lists three others).
- The trap pushes **PC and MSR**; the saved PC is the *next* instruction's,
  unlike Division Exception, Access Violation, Privileged Instruction and
  Breakpoint-on-Halt, which save the trapping instruction's own. `RETIL`
  returns. Traps that re-execute auto-clear SSP in the pushed MSR.
- **Breakpoint-on-Halt** substitutes a `HALT` for the first byte of the
  instruction; enabled in Trap Control (`10`), vector at IVT offset `40h`.
- **All** traps vector through the Interrupt/Trap Vector Table whatever the
  interrupt mode, so the table must be resident on a **4K boundary** and its
  pointer (control register `06`) set before anything can trap. Single-Step
  is at offset **`3Ch`**. Any patch needs a `PCACHE`.

| | NMI (implemented) | Single-Step trap |
|---|---|---|
| Setup in target memory | none — fixed `0066H`, handler injected | 4K-aligned IVT + pointer register |
| Saved status | PC only (modes 0-2) | PC + MSR |
| Return | `RETN` | `RETIL` |
| Timing sensitivity | must assert during the opcode fetch | none — fires by design |
| Block instructions | can interrupt mid-block; saved PC is the block instruction | steps over the whole block |

NMI does not reserve 4K of the target's physical memory. Its timing
sensitivity, knocked out once by an unrelated refresh divisor, is the main
argument for the trap route.

#### Bus-cycle tables from the chip **[hw]**

The Z-BUS cannot tell an opcode fetch from a data read and the prefetch unit
runs ahead, so no per-opcode bus-cycle listing can be read off the manual the
way TLCS90 and i8096's can. `scripts/record-cycles.py`, with
`tools/cycles_z280.py` as its plugin, runs every pattern of libasm's
`gen_z280.lst` (kept as `tools/gen_z280.lst.zst`) plus every other opcode
libasm decodes (`tools/z280-opcodes.txt.zst`) in isolation, with memory filled
with `FF` = `RST 38H`, and `tools/derive_tables.py` turns the recordings into
`z280-PAGExx.txt`, which `inst_z280.awk` turns into tables. The raw recording is
committed as `tools/z280-profile.jsonl.zst` (zstd, one JSON line per run, read
and written through Python 3.14's `compression.zstd`), so the tables can be
re-derived or the recording extended; they are hand-maintained from then on.
Recording needs the profile image: the plain build prints `R A=xxxxxx D=xxxx`
like the Z80's.

What the 2169 runs established:

- **Prefetch depth**: at most 3 words past an instruction before its last data
  cycle (0 in 40% of runs, 1 in 49%); 1-3 words at the exit. Sequences are
  stable across repeats.
- **A byte load at an even address is a word read**; at an odd address a byte
  read. A word at an odd address is two byte transactions.
- **Stalls and flushes repeat fetches**: multiply and divide re-read the next
  word up to five times; EI, DI, PCACHE, LDCTL and a few others re-fetch after
  a flush.
- **Traps** are three pushes, two table reads and the fetch (`WWRRS`). The
  harness's operands made every divide trap, so `derive` drops that tail from
  divide rows; `SC` keeps it.
- **RETIL** is two reads and the fetch (`RRA`); no `out(1)` as Table E-1
  suggests.
- Block instructions come out as `{...}` from one and two iterations, the
  second at the other parity.
- The relative and 16-bit indexed forms of the `FD ED` page and the register
  variants the pattern list leaves out were run as extra patterns; anything
  still without a row is filled by operand shape from a recorded opcode on the
  same page (IX and IY pages mirror).

The recording was redone after `RegsZ280::restore()` was fixed to put the
frame below the SP it resumes with, so that an edited SP holds; 177 runs saw
other addresses or operand data, none changed its cycle pattern. **[hw]**

#### Disassembling the ring **[hw]**

`disassembleCycles()` reconstructs the fetch stream from those tables, as
TLCS90 and i8096 do. `InstZ280::match()` walks a table's sequence against the
ring: the instruction's own bytes in order, then its data transfers, with
prefetch and any stall/flush re-fetch absorbed wherever it lands and left
unmarked, since it is the next instruction's fetch -- so the next match starts
right after the previous instruction's own bytes (the i8096 model). A taken
transfer ends at the target's fetch; an interrupt taken instead of the fetch
owed is matched as one.

A cut instruction at the ring's start decodes as whatever its tail bytes say,
since every start in a byte stream decodes plausibly. What tells them apart is
the chain: every real instruction is followed by the one it expects -- the next
address, the target it took, the return address it popped, an interrupt's
vector, or the PC the CPU stopped at. `matchAll()` rejects a match that chains
nowhere (the follower when it chains nowhere either, else the one before) and
matches again until the chain holds; `findFetch()` then takes the start whose
chain ends at the PC. A failed match undoes only its own marks: the
instruction before it may own data cycles inside its window.

`tools/check_samples.py` cross-checks every dumped line of the samples against
their listings; `test/z280/test_inst_z280` (`pio test -e native`) replays
captured dumps through the matcher on the host, checks every mark against the
listing, and with `Z280_DUMP=<file>` replays any dump and prints the marks,
under the sanitizers if wanted.

#### Samples **[hw]**

| sample | result |
|---|---|
| `echo.asm` | works — characters echoed, LF added after CR, clean exit |
| `arith.asm` | works — all 30 results correct for signed 16-bit |
| `echoir.asm` | works — interrupt mode 1 |
| `echoitr.asm` | works — interrupt mode 0, vectored restarts |
| `mandelbrot.asm` | works — 4.74 s per frame, loops until stopped |

The four that exit do so at their own `rst 38h`; `mandelbrot` loops until
Ctrl-Space on the console or any byte on the halt port, and a run that ends by
itself has failed. A frame is **4.74 s**, three consecutive frames measured
between the blank lines; runs reported as "131 lines in 64ms" had exited early.
Mandelbrot never finished a frame at the original 100 ns #XTALI phases; the
larger win was free-running `loop()`.

### Z80 / Z180

Byte bus, one byte per read, sequential-cursor injection — so the faked-POP
idiom (`POP rr` followed inline by its payload) works and is used throughout
`regs_z80.cpp`. Do not copy it to a word-bus target. **[code]**

`z180`/`z80` `completeCycle()` treats an I/O address as 8-bit and ignores the
high 8 bits, so bugs in the high byte of an I/O address are not observable
there. **[code]**

### TMS9900 family

**Pitfalls.** Assert the halt interrupt only after the opcode fetch, or the
step lands on the wrong instruction.

`pins_tms9900_base.cpp::suspend()` is the reference for that idiom: it walks
the loop with a `bool assert_nmi` latch and asserts on the first cycle where
`s->fetch()` is true. Targets without an `#M1`-equivalent substitute "the
transaction `resumeCycle()` handed back", which is the opcode fetch by
construction — every sequence parks on the read that follows it. **[code]**

The TMS9980, TMS9995, TMS99105 and TMS99110 also run TMS9900 code. **[hw]**

The TMS99105 board names its CPU by behaviour, not by marking: at reset an
injected `CIR` makes two writes when a floating-point macrostore answers.
The bench's chip, marked TMS99105, has the TMS99110's ROM on chip, so
`MACRO=STANDARD` reports TMS99110 and only `MACRO=BASELINE` runs it as a
plain TMS99105. The `?` banner reads `BionicTMS99105 (CPU: TMS99110)`;
take the CPU from the `CPU:` field, not the board name. **[hw]**

### 6502

**Pitfalls.** Power-cycle after swapping chips on the 6502 board: `R` does
not redetect the part, so the debugger keeps the previous chip's identity
and instruction set (a W65C02S showed as G65SC02 until power-up). **[hw]**
`Regs` needing memory gets `Mems*` through its constructor, and some
targets build `_regs` before `_mems`.

`RegsMos6502` takes its `Mems*` as a constructor argument rather than reaching
through `Pins`, because `Pins::_mems` is protected and only `Target` is a
friend. Follow that pattern when a `Regs` implementation needs memory access,
swapping the construction order first where `_regs` comes first. **[code]**

### F3850

**Pitfalls.** A Teensy interrupt mid-cycle desynchronises the ROMC sequence;
mask them around each bus cycle.

Mask Teensy interrupts around each bus cycle in `cycle()`. An interrupt landing
mid-cycle desynchronised the debugger from the ROMC sequence, which showed as a
stack pointer off by one and a nondeterministic mandelbrot. **[hw]**

Release the data bus right after `Cycles::next()`; the bus level holder keeps
the value for the CPU, so the turnaround does not need a wait. **[hw]**

The 16-bit compares in `arith.inc` must compute `m + ~s + 1` and derive the
flags from that; the earlier version got `32700 < 32600`. **[hw]**

The profile (`tools/cycles_f3850.py`, recorded as
`tools/f3850-cycles.jsonl.zst`) matched every opcode to the data sheet's
ROMC sequences. **[hw]** Its `x=` counts Φ periods: a short cycle is 4 and
a long one 6. `INS` and `OUTS` of ports 2 and 3 (A2, A3, B2, B3), which
Table 3 leaves out, take one short cycle as `f3850.txt` has them, not the
long I/O form the Guide to Programming implies.

### MCS-48

**Pitfalls.** Some machine cycles pulse only ALE with no strobe, and the
makers differ on which; an ISR switching register bank must not use the
other bank's pointers uninitialised.

One board runs the P8039/P8048 and OKI MSM80C35/MSM80C39. The OKI parts are
told apart by `DEC @R0`, which the Intel parts lack. The board identity is
`P8048`. **[hw]**

The stack lives in internal RAM 08h-17h, with SP in PSW's low three bits; each
call stores the return address with PSW's top four flags. **[doc]**

The fetch data drive is released about 300 ns before `#PSEN` rises. It is
harmless on the parts tried. **[hw]**

The profile (`tools/cycles_i8048.py` on the P8039 and
`tools/cycles_msm80c39.py` on the MSM80C39, recorded as
`tools/i8048-cycles.jsonl.zst` and `tools/msm80c39-cycles.jsonl.zst`) found
machine cycles that pulse only ALE, with no strobe, where the matcher had
expected one. **[hw]** `i8048.txt`'s `s` column gives the cycles that do
strobe, as Intel/OKI where the makers differ:

- `IN A,P1`, `IN A,P2`, `OUTL P1,A`, `OUTL P2,A` (09, 0A, 39, 3A) and the
  MSM80C39's `MOV P1,@R3` (F3) strobe once in their two cycles.
- `RET` and `RETR` (83, 93) strobe twice on the Intel parts, once on the
  OKI ones.

Counting a strobe that never came made the matcher take the next fetch for
the instruction's own cycle; on the MSM80C39 that handed the CPU a real
`HALT` and wedged the board.

The interrupt-driven samples' ISRs switch to register bank 1 and call the
queue routines, which push through R1 onto the software stack in external
data memory; `init` set only bank 0's R1, so the ISR pushed wherever bank
1's R1 pointed, RAM a reset leaves as it was. **[hw]** Landing in the main
program's stack, it overwrote values the arithmetic had saved there, and
mandelbrot drew its leftmost column wrong in the rows next to the axis,
whose first pixel is computed while the newline's characters go out.
The ISR has its own stack now, at `isr_stack`.

### MCS-51

**Pitfalls.** An 80C51 in idle mode makes no bus cycles, and a reset in the
middle of a run must be followed by `restore()`.

A P80C51 in idle mode holds ALE high; `prepareCycle()` gives up after
`idle_cycles` and resets the CPU rather than wait for ever. `resetPins()`
saves the registers by running injected code at 0000, which leaves the PC
where that code ended: without a `restore()` (`LJMP` to the saved 0000) the
CPU went on into whatever memory held there, which looked like leftover
code running at 003A. **[hw]**

`udiv16_8` in `arith.inc` dropped the ninth bit when shifting the partial
remainder, so any divisor of 128 or more with a dividend above 255 divided
wrong (`-30000 / -200 = 128`). When the shift sets carry, subtract
unconditionally. **[hw]**

R0-R7 are internal RAM 00h-1Fh, four banks picked by PSW's RS1:RS0; dumping
00-1F shows all of them. **[doc]**

### 8080

**Pitfalls.** Sampling SYNC or status before the datasheet's delays after
PHI2 rises reads a released bus and writes stale data over code.

`prepareCycle()` must not sample SYNC before tDC (150 ns max) or status before
tDD (220 ns max) after PHI2 rises. Checking SYNC at 60 ns occasionally missed
T1; the status was then read in T2 from a bus the CPU had released, some reads
looked like writes, and stale bus data was written over code. Frame 1 of
mandelbrot came out with a few wrong characters and frame 2 fell apart. **[hw]**

Compare memory with the loaded image after a run to tell bus errors from
program bugs: the corrupted bytes were all operands the CPU had been reading.
**[hw]**

The NMOS 8080 is dynamic (tCY max 2 us), but clock stretching was not the
cause here: a pulse-width trigger on PHI1 never fired during a failing run.
**[hw]**

### 8085

**Pitfalls.** RST 5.5/6.5/7.5 make no INTA cycle.

RST 5.5/6.5/7.5 are vectored inside the CPU: after a discarded opcode fetch it
pushes the return address and jumps to the fixed vector, with no INTA cycle.
The same memory comparison that found the 8080's corruption shows none here.
**[hw]**

### 8096

**Pitfalls.** A missing `#` silently assembles the direct form; INT_MASK bit
n is the vector at 2000H + 2n; a word access needs an even address.

`ldb INT_MASK, INT_EXTINT` without `#` assembles to the direct form and loads
register 40H; EXTINT was enabled only when that byte happened to have bit 7
set. The listing's opcode (`B0` direct vs `B1` immediate) shows the slip.
**[code]** Adding the `#` stopped every interrupt-driven sample: `INT_EXTINT`
was `1000000B`, bit 6, which enables the serial port's interrupt. INT_MASK
follows the vector table, bit n for the vector at 2000H + 2n, so EXTINT at
200EH is bit 7, `10000000B`. **[hw]**

The profile (`tools/cycles_i8096.py`, recorded as
`tools/i8096-cycles.jsonl.zst`) found a matcher hang and three wrong rows.
**[hw]**

- `PUSH [w]`, `PUSH n[w]` and `POP n[w]` prefetch once more between the
  operand read and the writes: `PUSH [w]` is `1:2:~:R:r:~:W:w`. `POP [w]`
  makes no such read.
- `DIV l,[w]` (`FE 8E`) prefetches before its operand read, as the other
  indirect forms do; its row had no `~`.
- When the short-index form of a `/` row failed, `InstI8096::match()` went
  on to an empty sequence, which matched without taking a cycle, and
  `matchAll()` looped on it for ever: a backtrace past any such mismatch
  wedged the board. It stops at the `/` now.

A word access needs an even address; the profile keeps `n + [w]` even.
The pointer register's LSB selects auto-increment for `[w]` and the long
index for `n[w]`, and the profile uses an even register, so the long-index
forms are not checked on the chip yet.

### SC/MP (INS8060)

**Pitfalls.** The printed PC is one byte early (see General); routine
addresses are `ADDR(label)`, one less than the label.

PC is incremented before each fetch, so it holds the address before the next
instruction. Routine addresses are loaded as `ADDR(label)`, and a call is
`XPPC P1` with the operands inline after it. **[doc]**

`divsi2` in `arith.inc` fell from its negative-divisor path into the exit
trampoline without dividing, and took the quotient's sign from the divisor's
low byte. **[hw]**

### INS8070

**Pitfalls.** Same early PC as the SC/MP; reset leaves P2, P3, T, E and A
as they were; `DIV EA,T` leaves no remainder.

Same PC convention as the SC/MP. Reset clears only PC, SP and the status bits
other than SA/SB; P2, P3, T, E and A keep leftover values. **[hw]**

`DIV EA,T` leaves the quotient in EA but not the remainder in T;
`print_uint16` multiplies back to get it. **[hw]**

`-vF` as a displacement is not "minus F" when `vF` is offset 0: mandelbrot
computed `B+Q*F` for `B-Q*F` and drew a symmetric but wrong frame. **[hw]**

`XOR A,d,PC` (`E0`) had no cycle sequence, so no backtrace matched past
it. The profile (`tools/cycles_ins8070.py`, recorded as
`tools/ins8070-cycles.jsonl.zst`) shows it reading its operand PC-relative
like the other PC-relative forms, `1:2:Q:N`. **[hw]**

### MC6800 family

**Pitfalls.** `WAI` stacks first and then idles, so a halt finds no push
(and on VMA parts no write count change); `#XIRQ` is level-sensitive; the
MC68HC11's `STOP` can bring E back at another phase.

Three boards run six chips: MC6800 (and MB8861), MC6802 (and MB8870), MC6801
(and HD6301). The first two share `samples/mc6800`; the MC6801 has its own
samples. The HD6301 and MC6801 also pass the MC6800 regress, running MC6800
code. **[code]**

`PinsMc6802` built a `MemsMc6800` and cast it to `MemsMc6802`, writing past the
object. **[code]**

Halting a CPU in `WAI` wedged the board past the halt port. **[hw]** `WAI`
stacks the context up front and then idles, which broke two assumptions:

- On the MC6800 and MC6802 the idle cycles have VMA low, which leaves
  `_writes` alone. It stayed at 7 after the push, so `loop()` took its
  context-save branch on every cycle and never polled the halt switch. The
  branch now clears `_writes`.
- The halt's NMI then fetches its vector without pushing anything, while
  `suspend()` waited for 7 writes that never came. A read of the NMI vector
  before the push now means `WAI`: `suspend()` injects `RTI` to resume after
  the `WAI`, then takes a fresh NMI edge. The wait is also bounded.
- Stepping a `WAI` put the push first and the vector fetch an unknown number
  of idle cycles later, but `suspend()` took the vector from a fixed cycle
  after the push. On the MC68HC11 the real `#XIRQ` fetch then landed inside
  the next step's injected `RTI`. `suspend()` now looks for the vector read,
  and releases the line only after it -- `#XIRQ` is level-sensitive, unlike
  NMI.

The MC6801 has no VMA, so only the second one hit it.

The profile (`tools/cycles_mc6800.py` and its siblings, with a recording
for each of the six chips in `tools/`) found `CPX #n16` taking four cycles
on the MC6801, `1:2:3:x:N` like `SUBD #n16`; its row had three. **[hw]**

The MC68HC11's `STOP` freezes E, and after the wake-up E can come back at
another phase against the EXTAL the debugger drives. The debugger never
looked at E after reset, so every later cycle was sampled at the wrong time
and the halt read garbage registers. `rawCycle()` now checks that E is high
where it must be; if not, it marks the cycle non-VMA and walks EXTAL until E
falls again. **[hw]**

### MC6805 family

**Pitfalls.**

- `WAIT` and `STOP` fetch nothing until an interrupt, so a halt must give up
  and take an IRQ; `STOP` restarts the bus clock at another phase.
- On the MC68HC08AZ0 the run loop follows each instruction's cycle sequence,
  so a wrong table row, or an interrupt taken over a prefetched opcode, loses
  the instruction boundary and with it halts and breakpoints.
- `MUL` and `DIV` re-read the next opcode's address: a step's `SWI` must go
  into every read of it.
- The HC08's COP watchdog is a mask option and cannot be disabled; its reset
  also turns off the internal read visibility the debugger needs, so the run
  loop sets it up again at the reset's vector read.
- With IRV on, the MC68HC05C0 drives the bus in its internal cycles, and
  not every one has an internal address: the extended-indexed (`a16,X`)
  dummy read, the third cycle after a `Dx` fetch, reads `hi:FE`. Driving it
  too fought the chip for about 80 ns, seen on a capture of `D0`; the
  debugger now counts cycles from the fetch #LIR marks and reads that one.
  **[hw]**

Three boards run three chips: MC146805E2 (`samples/mc6805`), MC68HC05C0
(`samples/mc68hc05`) and MC68HC08AZ0 (`samples/mc68hc08`). The HC05 and HC08
also run MC146805E2 code. The HC08 has its own run loop; the other two share
`PinsMc6805`. **[code]**

The halt clocks the CPU up to its next opcode fetch, and `WAIT` and `STOP`
fetch nothing until an interrupt, so halting in either wedged the board.
**[hw]** `suspend()` now gives up waiting after 64 cycles and takes an IRQ
-- both clear the I mask -- whose five-byte push stands in for the `SWI`
that `save()` would inject; stepping over `STOP`, once refused, goes the
same way. `WAIT` restarts the bus in phase, but `STOP` freezes the bus
clock with DS either high or low and restarts it at another phase against
the OSC1 the debugger drives, so after `STOP` the MC146805E2 walks OSC1
through a whole DS pulse first, as reset does. Which one it was comes from
the last opcode fetched, recorded as the loop runs: by the time of a halt
the ring has long lost it.

Stepping after a breakpoint inside an interrupt handler ran off into
internal RAM. **[hw]** `captureExtra()` injected the SWI vector without the
dummy cycle that follows it, which `save()` does include, so the CPU was a
cycle behind: the next injected `LDA #` lost its opcode to that dummy cycle
and its operand, the saved CC, ran as an instruction. In the handler that
was `FD`, `JSR ,X`; in the main loop it happened to be harmless.

`samples/mc6805` runs on the MC68HC05C0 too: `cputype.inc` picks the
ACIA at 17F8 or FFE0, and the samples carry both chips' vectors. **[hw]**
`echoitr` wrote the MC146805E2's ACIA control directly when enabling the
transmit interrupt, so on the HC05 its output only moved when a received
character's interrupt happened to drain the queue; it goes through
`store_ACIA_control` now. Mandelbrot is slow on the MC146805E2 (6 rows
take 23 seconds) and mid-speed on the MC68HC05C0 (7 rows in 20).

The MC68HC08AZ0 is the fastest of the family (22 rows in 20 seconds).
**[hw]** Its COP watchdog is a mask option: MORA (`$1F`) is read-only with
COPD clear, so a program that doesn't service it is reset every 2^18 OSC1
cycles. That reset also turns off the internal read visibility the debugger
sets up, which left halts and breakpoints blind; the run loop now redoes
`reset()`'s setup at the vector read and jumps to the program's vector. The
MC6805 samples ran only a line before going astray there. They run now:
their variables moved from `$40`, not RAM on it, to `$50`; the MC68HC05
samples go through `cputype.inc` too, whose `load_ACIA_status` services the
COP on anything but the 6805; and the loops that wait without reading the
ACIA service it themselves. `restore()` writes COPCTL (`$FFFF`) before
every run, as a captured write that leaves the emulated reset vector alone,
so each run starts a full COP period.

The HC08's run loop follows each instruction's bus-cycle sequence from
`mc68hc08-P*.txt` to find the next opcode fetch, where a halt injects its
`SWI`. `scripts/record-cycles.py` with `tools/cycles_mc68hc08.py` recorded
every opcode on the chip, on the profile image, cross-checked by a
logic-analyzer capture the debug pin frames, and the tables match it now.
**[hw]** `DIV` claimed three next-opcode fetches where the chip makes one,
so the loop lost the instruction boundary after every `DIV`. Where AN2627
gives an HC08 instruction more program fetches than bytes (`TXS`'s `pp`,
`MUL`'s `ppddd`), the extra ones re-read the next opcode's address or the
byte after it -- `TXS`'s second reads past the next opcode, which only the
chip showed -- and the tables' dummy reads are right for those. The loop
also started a cycle late: `run()` completes the first opcode fetch, and
the loop took the operand after it for the opcode. Both put halts and
breakpoints at the wrong cycles: `haltgo` had wedged on one of ten halts,
and now runs 50 of 50 on both sets of samples.

An interrupt taken over a prefetched opcode lost the boundary too, and was
what stalled mandelbrot. **[hw]** The CPU drops the prefetch and stacks five
bytes, but the loop only saw an interrupt in writes at an instruction
boundary, so a `DIV` prefetched in `udiv16` swallowed the stacking and the
vector read. The loop then took the receive handler's ACIA status read for
a fetch, decoded the status, `$82`, as an illegal opcode, and spun without
clocking the CPU; a key press made the status `$83`, an `SWI`, and let
mandelbrot go on. Five writes in a row inside any sequence but `SWI`'s now
mean an interrupt, and an opcode without a sequence halts with the cycles
that led to it.

Stepping `DIV` ran off. **[hw]** `MUL` and `DIV` read the next opcode's
address again after the prefetch and take the opcode from that read, so
the step's `SWI`, injected into the prefetch alone, never ran; it goes into
every read of that address now.

`WAIT` and `STOP` make one bus cycle, the next opcode's prefetch, and then
none until an interrupt or a reset. **[hw]** The loop tells that from
eight bus cycles of silence rather than from the opcode, keeps the devices
running while the CPU sleeps, and on a halt takes an IRQ whose push stands
in for the `SWI`, as the other two chips do. A reset ends a sleep, or a
run, with its vector read, and the loop realigns there: a program that
sleeps without a periodic interrupt is reset by the COP every 2^18 OSC1
cycles. Stepping `WAIT` or `STOP` on the HC08 is refused. **[code]**

The profile (`tools/cycles_mc146805e2.py` and `tools/cycles_mc68hc05c0.py`,
recorded as `tools/mc146805e2-cycles.jsonl.zst` and
`tools/mc68hc05c0-cycles.jsonl.zst`) matches the tables on both chips.
**[hw]** Each pattern is padded with `SWI` to four bytes, longer than any
instruction: a shorter pattern written over a longer one left the earlier
one's bytes after it, and on the MC68HC05C0 seventy runs went on into them.

### MC6809 family

**Pitfalls.** `CWAI` stacks before it waits, so a halt finds no push; the
MC6809 has no VMA (its CNTL0 is XTAL); the 6309's vector base is FFF0; R/`#W`
must be read as E rises on the HD6309.

Two boards run four chips: MC6809 (and HD6309), clocked from EXTAL, and
MC6809E (and HD6309E), whose E and Q the debugger drives. The HD6309 runs
MC6809 code and has its own samples, which run in native mode. **[code]**

Halting a CPU in `CWAI` returned garbage registers. **[hw]** `CWAI` stacks
the whole context and then waits, so the halt's NMI fetches its vector
without pushing; `suspend()` looked back from the vector read for the push,
found none, and captured an empty frame. On an HD6309 in native mode that
also cleared MD, dropping the CPU to emulation mode on the next resume.
`suspend()` now injects `RTI` to resume after the `CWAI` and takes a fresh
NMI edge, and its wait for the vector is bounded. `SYNC` never needed this:
it stacks only after the interrupt ends the wait.

No backtrace or `G` dump was ever disassembled on the MC6809, for two
reasons. **[hw]**

- `Signals::getDirection()` took its valid bit from CNTL0, which is AVMA on
  the MC6809E but XTAL on the MC6809. XTAL reads low while the debugger
  drives EXTAL, so every cycle looked invalid and no instruction matched.
  The MC6809 has no VMA at all; its dummy cycles read FFFF, which the
  cycle sequences already match as `N`, so the valid bit is now forced on.
  AVMA is no use for it either: it tells of the next cycle, not this one.
- `InstHd6309` kept the MC6800's vector base of FFF8, so the FFF6 fetch of
  every `FIRQ` failed to match, and with it any backtrace across one --
  the samples drive the ACIA on `FIRQ`. It is FFF0 now, as on the MC6801
  and HD6301, which also covers `SWI2`, `SWI3` and the HD6309 trap.

The MC6809E marked fetches from LIC, shifting each mark one cycle on as
the next opcode fetch. That missed often enough -- around halts and
interrupts, and on a second pass over the same ring -- to put a mark on a
dummy FFFF cycle. It now finds fetches by matching cycles, like the
MC6809. **[hw]**

That stray mark wedged the board, which led to a bug in
`Mems::disassemble()`: at FFFF the instruction runs past memory, and
NO_MEMORY left its length 0, so it returned the address unchanged. Every
backtrace that steps by the returned address then re-disassembled FFFF
forever, flooding the console and ignoring the halt port. It now steps at
least one unit. **[code]**

When profiling, the HD6309 drives R/`#W` back high right at E's fall, so
read R/`#W` as E rises; compare only the kind of a dummy cycle such as the
read of `$FFFF`. **[hw]**

`CWAI` and `SYNC` halt and step correctly in both emulation and native
mode. Mandelbrot is mid-speed on the MC6809 and MC6809E (6 rows in 20
seconds) and fast on the HD6309 in native mode (a frame in under 10
seconds).

### SCN2650

**Pitfalls.** A conditional branch not taken skips its indirect address
read, two cycles fewer than its row; `HALT` makes no bus cycle after its
fetch.

The profile (`tools/cycles_scn2650.py`, recorded as
`tools/scn2650-cycles.jsonl.zst`) found that a conditional branch not taken
reads no indirect address: an indirect `BCTR`, `BDRA` and the like makes
two bus cycles fewer than its row when it falls through. **[hw]** The run
loop and the matcher took those two cycles from the next instruction;
`InstScn2650::notTaken()` now tells a fall-through by the address after the
operand.

### TLCS90

**Pitfalls.** On-chip RAM and I/O (FEC0-FFEF) never show on the bus, so
instructions touching them make fewer cycles than their rows; a word access
at FFFF wraps to 0000.

The profile (`tools/cycles_tlcs90.py`, recorded as
`tools/tlcs90-cycles.jsonl.zst`) checked every opcode on the chip against
the manual's bus-cycle tables, and found two matcher errors. **[hw]**

- `JRL` and `CALR` took their 16-bit displacement from the opcode + 3; it
  counts from the opcode + 2, as an 8-bit `JR`'s does.
- A word access at FFFF goes on at 0000, which `r` and `w` did not allow.

On-chip RAM (FEC0-FFBF) and I/O (FFC0-FFEF) never show on the bus; FFF0-FFFF
is external again. The profile keeps its operands and SP in external memory,
and leaves out patterns that would point SP at on-chip I/O. **[hw]**

### TMS320C15

**Pitfalls.** The bus timing has no slack: the 10 ns delays that replaced the
commented-out debug pin calls are the setup margin; don't remove them.

The profile (`tools/cycles_tms320c15.py`, recorded as
`tools/tms320c15-cycles.jsonl.zst`) matches the tables on every opcode.
**[hw]** The board holds `#BIO` high, so only the fall-through of `BIOZ` is
recorded.

Mandelbrot drew a few wrong pixels, at the same places but not every
frame. **[hw]** The fetch data settled as late as the CLKIN edge after it:
the commented-out debug pin toggles had been the only setup margin. 10 ns
after each drive and before each sample settles it 40 ns before that edge
and 110 ns before `#MEN` rises, holds it 120 ns after, and the frames come
out right, with analyzer leads on the bus too, at 5.4 s a frame against
4.6 s without.

### TMS7000 family

**Pitfalls.** Register-file accesses, pushes and pops included, never reach
the bus; `TRAP n` makes two bus cycles.

One table serves the whole family, and the profile
(`tools/cycles_tms7000.py`, recorded on a TMS70C02 as
`tools/tms7000-cycles.jsonl.zst`) takes any family member. **[hw]**
`TRAP n` makes two bus cycles, its vector reads, which `tms7000.txt` had
as none.

### PDP-8 (IM6100 / HD6120)

**Pitfalls.** The debugger's `restore()` ends in `RTF` on both chips, and
on the IM6100 `RTF` enables interrupts after the next instruction whatever
IEFF was. **[hw]** Resuming inside an interrupt handler whose request was
still pending took the interrupt again: the CPU wrote its return address
into location 0000, over the program's, and ran off. The debugger now holds
INTREQ off while the saved IEFF is clear, records IEFF as clear in `save()`
until then, and lets requests through once the program fetches its own
`ION` or `RTF`. The HD6120 is not affected: stopped inside `echoir`'s
handler with the request pending, stepping and `G` kept interrupts off
until the handler's own return, and location 0000 kept the main-line
return address.

# Development style

Notes on bringing up targets in this codebase: what is not obvious from the
source.

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
desynchronises a cursor at once. **[hw]**

A faked `POP` — the opcode followed inline by its payload — works only under a
cursor (the Z80's); under address-keyed injection the stack read lands at SP
and is answered from real memory. **[hw]** Use real load instructions there;
where a register has no load (typically a flags register), stage the value in
memory and point SP at it.

### Terminating an injected sequence

Do not count bytes handed over: one word read can deliver two single-byte
instructions, and the CPU's next read is then answered from memory, handing
it an opcode that is not in the sequence. **[hw]**
Terminate on *the CPU having read past the window* instead.

The CPU prefetches past the end of a window before it takes a trailing jump,
and that prefetch looks exactly like a forward exit. If a sequence must end by
jumping somewhere specific (its own origin, a restored PC), the caller has to
say so. The same prefetch means a sequence "exits" before its last instruction
has run: anything that depends on that instruction must wait for it (the Z280
ends such sequences with a taken jump). **[hw]**

### Pipeline flushes force re-fetches

An instruction that flushes the prefetch queue, and interrupt and trap
processing, make the CPU re-fetch addresses it has passed. Keep an injected
window answered across those re-fetches, not just until its last byte has gone
out. Serve an injected interrupt handler's vector for as long as the CPU asks
for it. **[hw]**

### Resume from an explicit origin, never from the ring **[code]**

A CPU parked mid-transaction is resumed at the address it was parked at. That
address cannot always be re-read from the pins (a multiplexed bus carries data
by then), and it cannot live in the ring: `Cycles::reset()` and
`Cycles::discard()` both clear the head slot, and a dump or an exit is exactly
when they run. Follow the i8080/i8085 shape: `resumeCycle(addr)` takes the
address, the caller keeps it, and `_regs->nextIp()` is where the CPU is parked
between operations.

**The ring holds one fewer cycle than it has slots.** The head slot is the
transaction in progress. **[code]**

**A run's dump shows the program, not the debugger.** The break unwinding, the
halt's interrupt push and vector fetch, and every injected read and captured
write of `_regs->save()` stay out of it: discard what an exit injected, and
either dump before saving (as `step()` and the Z80 do) or hold the ring while
saving (the Z280). **[code]**

### Keep bus-keepalive cycles out of the ring **[code]**

Refresh transactions keep DRAM alive; at debugger clock speeds one can land
between almost every pair of program transactions. Recording them:

- breaks `s->prev()`, which callers use to mean "the previous *program*
  transaction" — e.g. matching an interrupt vector fetch against the PC push
  that must immediately precede it;
- floods the 128-entry ring so a dump shows nothing else.

`completeCycle()` does not advance the ring for them. That, not disabling
refresh at the source, is the fix (the Z280's refresh cannot be disabled).

### Bound every wait loop **[hw]**

`loop()` only polls the halt switch *between* steps, so a wait loop that never
returns cannot be broken into from the halt port. Give every "wait for the CPU
to do X" loop a guard and a failure path.

The RTWDOG backs this up: the prompt, every completed bus cycle and every halt
switch poll feed it, so a loop that does none of them reboots the Teensy after
4 s, and the banner reports the watchdog reset. A reboot still loses
breakpoints and the target's state, and a loop that keeps completing bus
cycles is not caught, so bound the loop anyway. The profile image's `X`
command hangs on purpose, to test it. **[code]**

### The halt port **[hw]**

The Teensy builds with `USB_DUAL_SERIAL`. `/dev/ttyACM0` is the debugger
console; **any byte written to `/dev/ttyACM1` aborts a running CPU**
(`serialEventUSB1()` → `Pins::isrHaltSwitch()` → `_halted`, polled by `loop()`).
It is the only way to stop a run that does not end on its own — provided the
wait loops are bounded.

**It is serviced only from `yield()`.** **[code]** A run reaches `yield()`
otherwise only through console I/O, so `Pins::haltSwitch()` calls `yield()`
itself; without it a program that never enables the console device cannot be
halted. A halt that never returns the CLI looks exactly like a wedged board, so
try a program that uses the console before suspecting the board. **[hw]**

**The handler must drain the byte.** The core re-calls `serialEventUSB1()` from
every `yield()` while the port holds data, so an undrained byte also aborts
every following run. The symptom is every `G` returning at once with a
register dump. **[hw]**

**An abort leaves the emulated USART repeating.** After a run is stopped
mid-cycle, the USART keeps re-delivering its last received character, which
floods `ttyACM0` and buries the CLI prompt. A harness drains the console after
any abort, and stops an interactive sample the way the sample expects (NUL for
`samples/z280`) rather than aborting it. **[hw]**

Recovery order for an unresponsive board, in increasing violence: Ctrl-C
(`0x03`), Ctrl-Space (`0x00`), then the halt port. If none draws a reply, the
firmware is stuck in a wait: the watchdog reboots it within 4 s if the wait
feeds nothing, and otherwise only a reflash brings it back.

### Lock a bench before using it **[hw]**

Several benches can be on USB at once, each a Teensy named `bench1`,
`bench2`, ... in the order first seen (`scripts/bionic-bench.py rename` gives
a clearer name), so several sessions work side by side, one bench each. A
session takes one with `scripts/bionic-bench.py lock NAME`, or queues with
`reserve`; `bionic-control.py`, and every tool built on it, then talks to the
bench the session holds, and refuses one another session holds. The bench's
two USB serials, console and halt port, follow from its Teensy's serial
number.

A logic analyzer is locked the same way. Each is a resource added with
`scripts/bionic-bench.py add NAME kind=logic mcp=URL`, its Logic 2 MCP
server; `logic-analyzer.py` captures on the one the session holds (or
`BIONIC_LOGIC`, or the only one) and refuses one another session holds.
Logic 2's MCP tools called directly cannot check: lock the analyzer first.
A capture it starts keeps at most 10 ms and stays open for a look until
the next start closes it; one that never triggers is stopped, not
exported, and an export takes only that capture's channels.

Powering a board off drops its Teensy off USB; a flash passes through the
HalfKay bootloader and a firmware crash re-enumerates within a second or two.
`scripts/bionic-bench.py daemon` logs each change to `bench-events.log`,
then the target seated, so a seat needs no reply in chat. Its look at the
target is the first open after a re-enumeration, the only one to see a
CrashReport, so it keeps the report and logs where. The session holding a
bench follows it with `scripts/bionic-bench.py watch --mine` under a Monitor
rather than polling `status`.
`scripts/bionic-bench.py peek NAME` follows what a bench prints to whichever
session drives it, without touching it.

### Halt through an entry the CPU takes itself

**An injected save races a pending interrupt.** A save injected at an ordinary
opcode fetch starts at an instruction boundary where the CPU may take a
pending interrupt instead and feed the injected bytes to its entry (the Z8,
TMS370 and TMS7000 all did). Polling the halt port less often makes this
common: thin the poll to provoke the race when testing a halt (see the
stress build below). **[hw]**

**Prefer an NMI, TRAP or LOAD input, or an injected trap that masks
interrupts.** The CPU then finishes the instruction, settles priority with any
pending interrupt itself, stacks one coherent frame including the program's
interrupt mask, and masks itself; the debugger serves the vector and captures
the frame. Resume with the instruction that restores the mask together with the
PC (`RTI`, `RETI`, `RETN`, `IRET`), or `EI` right before the jump. The MC6800
family, MC6809, MOS6502, 8085, Z80 family, Z280, Z380, TLCS90, TMS9900 family
and Z8000 halt this way. **[code]**

- An injected `SWI` needs the opcode fetch marked by hardware (`SYNC`, `M1`,
  `LIR`, `IAQ`, `#OCF`) and a check of which vector was read: a pending IRQ can
  still win that boundary and discard the `SWI` (MC68HC08). **[hw]**
- A trap that acts like a call (TMS7000 and TMS370 `TRAP`, 8080 `RST`) neither
  masks nor stacks the status, so it is no better than an injected save.
  **[doc]**
- An NMI that acts like a call (i8096) stacks only the PC and leaves the
  next instruction boundary open: an interrupt taken there reads its vector
  in place of the save. Serve it from memory and save when its handler
  returns to the park. **[doc]**
- Its push reads no vector, so the backtrace's ring must end before the
  push, as it ends before an 80C196's push and vector read; a ring ending on
  a bare push fits no interrupt sequence, and none of it disassembles.
  **[hw][code]**
- Every NMI-class input is a Teensy output, and the emulated devices only change
  interrupt pins from `devs->loop()`, which does not run during a save or
  restore. Where every interrupt source is the firmware (PDP-8), holding the
  request off from the halt to the program's first fetch after the restore is
  enough. On-chip peripherals cannot be held off this way. **[code]**

Where none of this is available (8051, 8048, F3850, SC/MP, INS8070, SCN2650,
CDP1802, Z8, TMS7000, TMS370, TMS320C15, MN1613), the save reads the interrupt
mask before setting it, and the restore enables interrupts in the same
instruction as the jump.

### Stress `haltgo` with the `teensy41-stress` build **[hw]**

With `yield()` on every halt poll, a halt rarely reaches the register save with
an interrupt pending, so a passing `haltgo` proves little. The
`teensy41-stress` build (`BIONIC_ENV=teensy41-stress` for
`bionic-control.py flash`) adds `Y n`: `yield()` on one in `n` halt polls.
`Y 1024` makes the pending interrupt common; a power cycle puts it back to
`Y 1`. The test is the regress plus five runs of `haltgo=30` at `Y 1024`.

Every family in "Halt through an entry the CPU takes itself" passes it, by
the entry or the masked save named there, with the remedies in its own section
where one was needed. The exception is the TMS9980 family: on its PWM clock
the TMS9980 and TMS9981 have passed only the plain regress.

### Compare a pin read against `LOW`, never `HIGH` **[code]**

`digitalReadFast()` may return the masked register bit, `1 << bit`, rather
than 1. That is truthy but **not equal to `HIGH`**, so `read() == HIGH` fails
on pins whose bit position is not 0. `LOW` is 0, so `== LOW` and `!= LOW` are
exact. Write the test that way even where the current expansion is safe.

### Commented-out debug pin calls still count as time **[hw]**

`assert_debug()` and `negate_debug()` cost about 10 ns of pin writes each, and
bus code written while they were live took that time as margin. The normal
build comments them out, so put the time back where it mattered: a
`delayNanoseconds()` of one call's worth after a bus drive, for setup before
the clock edge that latches it, and before a sample, for the signal to settle.
A delay on the other side only stretches the cycle. Wrong output (wrong
pixels, say) after the debug calls are commented out points at this lost
margin.

### `delayNanoseconds()` is not the phase it makes **[hw]**

`delayNanoseconds(n)` works out its cycle count at run time from
`F_CPU_ACTUAL` and spins on `ARM_DWT_CYCCNT`, and the pin writes around it take
time to reach the pin, so a clock phase built from it runs 10–25 ns longer than
`n`, and below about `n` = 20 it gets no shorter. How much depends on the code
around it: inside a tight loop the compiler hoists part of the arithmetic out.
On the i8096 at 100 MS/s:

| Phase | Measured |
|---|---|
| `hi(); lo();` with nothing between | 20 ns |
| `hi(); delayNanoseconds(20); lo();` | 45 ns |
| `hi(); delayNanoseconds(5); lo();` | 45 ns |
| `lo(); delayNanoseconds(10); hi();` | 35 ns |
| `lo(); delayNanoseconds(15); hi();` inside the ALE wait loop | 25 ns |
| `hi(); delayNanoseconds(10);` plus one pin read, then `lo();` | 38 ns |

- To meet a datasheet minimum, size `n` from a capture, not from the minimum
  itself: asking for the minimum overshoots it, and every overshoot is lost on
  each bus cycle.
- Write the measured phase next to the constant (`= 5;  // 30 ns`), and
  re-measure when the code around it changes.
- Work done inside the phase (a pin read, a port read, a bus drive) counts
  toward it, often enough to need no delay at all.

### Squeezing a Teensy-clocked bus cycle **[hw][code]**

Where a bus cycle's time goes is found by measuring, not by reading the code.
The S80296SA's went this way:

1. Capture XTAL1 with the bus strobes and time each phase from one bus state
   to the next. The phases the software stretches stand out against the
   ones the delays make; there each bus cycle lost about 650 ns twice.
2. Stamp the suspects with `ARM_DWT_CYCCNT` into a scratch buffer, print it
   after the run, and drop the stamps afterwards. A stamp costs about 50 ns,
   so subtract it.
3. A `yield()` on every cycle, from the halt switch's poll, cost about
   160 ns: poll it every 16 bus cycles instead.
4. A memory read through the virtual chain (`Mems::read`, the arch's
   `read`, `Devs::isSelected`, each device's `isSelected`, `read_byte`)
   cost about 350 ns, twice for a word: check the devices once for the
   aligned word and call `DmaMemory::read_word`/`write_word` directly.
5. Serve a special address (the CCB) where the CPU reads it (the reset),
   not with a compare in every read.
6. Trim a clock phase only after measuring how late the CPU's outputs
   follow its edge: the S80296SA's followed a rising XTAL1 by 10-22 ns and
   were sampled 60 ns or more later, so its high phase needs no delay.
7. Do the work as soon as its input is valid, not where the old code
   did it. The CDP1802 drives the low address byte from early c30, a clock
   before the code read it, so the memory read moved into c40 and the byte
   is staged in the port with `busWriteFast()` while the bus is still an
   input; c61 only turns the port to output.
8. Give every branch of a phase its own delay. A debugger-injected read
   skips the memory lookup, and with no delay in its branch the CDP1802's
   c40 low half shrank to about 30 ns: the CPU kept fetching the byte
   staged before, while the firmware went on as if each fetch were served.

### Clocking a run from a timer **[hw][code]**

A CPU whose clock may stop can run from a free-running FlexPWM output
instead of bit-banged edges, which is the way to a CPU whose clock goes
through a PLL. The MC68HC16 does this on EXTAL (P47); reset, step and
injected sequences stay bit-banged. The TMS9980 and TMS9981 run every
sequence from it on CKIN (pin 5), the TMS370Cx5x on CLKIN (pin 5), and
the INS8060 and INS8070 on XIN (pin 5), and the 8085 on X1 (pin 6), since
their clock must not stop. The SCN2650
is static, but runs from it on CLOCK (pin 5) too: the timer paces the
cycles, where bit-banged edges needed a tuned delay for each.

- Hold a bus cycle with the CPU's wait input (`#DSACK`, READY, `#OPACK`,
  `#HOLD`) instead of stopping the clock: withhold it until the cycle is
  served, and negate it as soon as the strobe ends, before the next cycle
  samples it. Mask interrupts from asserting it to negating it: an
  interrupt can outlast the gap between strobes, and the next cycle then
  ends unserved.
- `PwmClock<PIN>` (`debugger/pwm_clock.h`) does this for a FlexPWM
  pin: the pin fixes every register at compile time, so a call costs no
  more than the bare register access, even in a per-cycle loop. A new
  clock pin adds its row to `PwmPin` from the Teensy core's `pwm.c`.
- Hand the pin between GPIO and FlexPWM with the output masked. MASK
  double-buffers, and with `CTRL2[FORCE_SEL]` on the local reload it
  takes effect at a period start, so a pulse is never cut short: mask,
  wait two periods, then switch the pad mux, or switch the mux, then
  unmask. Never poll the counter for its low half: at a few MHz a read
  and a mux write no longer fit in it.
- A pad feeds GPIO or FlexIO, never both, and SION doesn't help either
  way: muxed to FlexIO, GPIO reads it as 0; left on GPIO, FlexIO reads 0.
  A FlexIO1 PIN read takes about 40 ns, too slow to poll a strobe at
  10 MHz. A FlexIO latch needs its own pad for the signal.
- `K [kHz]`, which `?` doesn't list, sets the rate at run time, for a
  sweep with the regress; a run starts at the target's constant.
  `clockInput()`, `setClockInput()` and the rate member stay under
  `#ifdef TUNE_CLOCK`, which every Teensy build defines, so a build
  without them is still possible; timeouts are sized for the slowest rate
  the CPU takes, constants that hold at any rate. `K` may go past the
  CPU's rated clock, to find where it stops keeping up: the SCN2650's
  goes down to halves of 200 ns.
- A CPU that samples its wait input on a clock edge needs it moved only
  outside that edge's setup and hold window, at its next sample too: the
  TMS9980 takes `READY` up to 2tw+10 after the `#PHI3` rise, so raised
  just after a rise it is seen a clock early and the next cycle ends
  unserved. Measure where each change lands in a capture.
- With the clock free-running, every read of the bus must follow the
  strobe or clock edge that makes it valid; one taken whenever, as a
  stopped clock allowed, catches the address changing.
- A CPU that synchronises its bus grant to its clock (the INS8060's
  `#ENIN`) waits up to a clock more for a grant that comes after its
  request, and at some rates the Teensy's fixed reaction just misses the
  sampling edge, so a faster clock runs slower. Grant ahead of the
  request, for a bounded wait, then fall back to waiting for the request,
  which a long instruction leaves unasked. Compare each phase of a cycle
  at two rates in a capture.
- A bit-banged serial handler paced by clock edges paces by the cycle
  counter instead: `SerialHandler::setClock()` and `pace()` tick once per
  prescaled count of the free-running clock, keeping the phase but
  dropping a long absence's ticks.
- A cycle the CPU does not hold has to be caught as it passes: the
  8085's acknowledge of TRAP and RST 5.5-7.5 has no strobe and ignores
  READY, so waiting there for a strobe takes the next cycle's. Latch each
  cycle and wait for its strobe in one masked stretch, with a bound of a
  clock or so; a cycle with none is recorded, and the next one latched.
- The speed is a staircase, not a slope: the CPU samples its wait input
  once a clock, so a fixed Teensy service time costs a whole extra wait
  state wherever it misses an edge. The 8085 ran slower at 10 MHz X1
  than at 9.375 MHz until the halt switch, whose `yield()` took about
  120 ns before every fetch, was polled once in 10 us instead. Measure
  each phase of a cycle in a capture before blaming the rate.
- Holding every cycle costs a wait state each. In a run, a plain memory
  read can go without one: decide from the status as soon as it is valid,
  leave the wait input released, stage the data in the port while it is
  still an input (`busWriteFast()`, no read of the data register), and
  drive it the moment the read strobe falls. Every 10 us hold a fetch to
  run the devices, which may change an interrupt input only while the
  CPU waits, and to poll the halt switch. It only pays while the Teensy
  keeps up: on the 8085 at 10 MHz X1, an ALE came 90 ns after a held
  write's strobe and was over before the Teensy looked, and zero-wait
  reads worked only at 6 MHz, slower than every cycle held at 10 MHz.
- Measure the bit-banged machine cycle against the CPU's rated clock
  before moving it to a timer. The CDP1802's bit-banged clock already
  ran near its 5 MHz, a 1.73 us machine cycle against 1.60 us, so a timer
  could gain at most that gap; holding each cycle with `#WAIT` made it
  2.39 us. Squeeze the bit-banged cycle instead. The CDP1802 pauses on any
  falling clock edge with `#WAIT` low, ignoring that edge; the CDP1804A
  family pauses only as TPA or TPB rises.
- A held cycle costs a wait state wherever the Teensy's service overruns
  the CPU's first wait sample. The Z8400A at 4.05 MHz samples `#WAIT`
  about 160 ns after `#MREQ`, and the Teensy needed about 300 ns from the
  strobe to the release, so every cycle took one more state and the Z80
  stays bit-banged. A DWT profile of the run loop's phases (address read,
  lookup, data out, bookkeeping) shows where the time goes without a
  capture.
- A strobe can trail its request by part of a clock: the HD64180S drops
  `#WR` 50-60 ns after `#ME`. Wait about a clock for it before deciding a
  cycle has none; deciding at once released a write unheld, and it was
  lost.
- Keep bookkeeping off the release: poll the strobe's end straight from
  the port, and advance the cycle ring once the wait input holds again.
  With the ring advanced first, a Z8018010 at 16.67 MHz EXTAL let an NMI's
  push run unheld in 4 of 6 steps, and every Z180-family chip ran slower.
- On an ALE bus the next ALE may follow a strobe by tCL, 50 ns on the
  NSC800, so slow work after the strobe misses it and the next cycle
  waits for an ALE that has passed. A write held before `#WR` already has
  its data out from the CLK rise after ALE: write the memory or device
  then, while held, and advance the cycle ring while the strobe lasts.
- Resuming a run serves the fetch the CPU waits in by its own status,
  read from the bus: a stale ring slot that said "write" stored the bus
  into memory at the PC.
- A multiplexed address needs its settle time even on a held cycle: the
  KL5C80A12's A16-A19 come through the ASEL mux about 20 ns after it
  switches, and read sooner they sent CALL operands to the wrong page.
- A target moved to a timer adds its row to the table in
  [hardware.md](hardware.md#clocking-the-cpu) (clock input, the input that
  holds a cycle, default rate) in the same commit, and a cast recorded on
  it notes `PWM` and the rate in the mandelbrot table.

### Driving a high-voltage clock **[hw]**

The TMS9900 (four phases, 12 V), P8080 (two phases, 12 V) and CP1610 (two
phases, 11 V) take clocks the Teensy's 5 V outputs can't reach, and the
phases must never overlap.

- The TMS9900 and P8080 boards level-shift each phase with a common-gate
  2N7002 (gate at VCC, source driven by 5 V logic) and a 750 Ω pull-up to
  the 12 V rail: the pull-up alone makes the rising edge.
- A dual low-side gate driver powered from the CPU's VDD drives both edges
  instead. The CP1610 board uses a UCC27324, with 33 Ω in series with each
  output to damp ringing into the 20-30 pF clock inputs. The UCC27444 and
  UCC27624 fit the same footprint: their pins 1 and 8 are enables pulled
  up inside, so a board leaving them unconnected takes either.
- The UCC27624 is the best of the three: its delay is the same for both
  edges (17 ns typical, 27 ns max) and matched between channels within
  2 ns, so the phases keep their widths and gaps; the UCC27444 delays a
  falling input 12 ns more than a rising one. For four phases use two, and
  pair in one chip the phases whose gap is tightest.

### A logic analyser on the bus can break the board **[hw]**

Probe leads add enough capacitance to stop a marginal target working at all
(sixteen leads on the Z280's multiplexed bus did). If a known-good commit
fails with the probes on, unplug them before suspecting the code.

- **Bisect against a known-good commit before believing any code theory.** With
  the probes on, every change looks like the culprit. `git reflog --date=iso`
  dates the builds, so a session's passing run identifies which commit to try.
- **Timing measured with the probes on is not the unloaded timing.**
- **A reading taken with probes missing on the signals that carry the answer
  is not evidence.**

### Bus level holders make the data bus turnaround uncritical **[hw]**

The port 6 pins **P6.16 to P6.31** — the data bus, among other uses — each
carry a **bus level holder**. When the controller switches one from output to
input, the level it drove is *held* on the net until the CPU drives the pin.
The data bus never floats across the turnaround: a CPU still sampling between
`inputMode()` and taking the bus reads the level the controller last drove.

- Releasing the bus need not be timed precisely against the point where the
  CPU stops sampling.
- A read of a pin in that range shows what is *held* there, not that the CPU is
  driving it. Do not infer bus ownership from a level.

### Verify opcodes with the assembler, not by hand

libasm ships a CLI assembler (`asm`, built from libasm's `cli/`); use it rather
than hand-encoding injected sequences, especially for relative jump
displacements:

```
printf '        CPU Z280\n        ORG 0\n        JR 0000H\n' > x.asm
asm -C z280 -l /dev/stdout x.asm -o /dev/null
```

Opcode questions are settled from the tables (`libasm/src/table_<cpu>.cpp`).

### Console gotchas when loading a test program **[hw]**

- `M`/`m` (program/data memory) accept at most **16 bytes per line**. A longer
  program truncates silently and the untouched tail executes as whatever was
  there, which reads as "the CPU got that instruction wrong". Split the load
  across several `M` commands and dump it back before drawing any conclusion.
- `M`/`m` always *write*; `p`/`d` dump. `M<addr>` followed by an empty line still
  writes, so a mistyped dump can corrupt the byte you meant to inspect.
- `b` (list breakpoints) prompts "clear?" and swallows the next command.
  `B<addr>` sets one without prompting.

### Read the manual before blaming the code

Grep the manual first; `pdftotext -layout` makes the PDF greppable. A measured
behaviour that contradicts an assumption, or a fix that should work and does
not, is usually explained there.

### Check cycle tables against the chip, not only the manual **[hw]**

Halt and breakpoint logic that follows bus-cycle sequences is only as right as
its tables, and the manuals can be wrong or ambiguous (the MC68HC08's AN2627).
`scripts/record-cycles.py` runs one instruction at a time on the profile image
(`-D PROFILE_CYCLES`, the `teensy41-profile` environment, one build for every
target), in memory filled with the target's trap so any transfer stops at once,
and compares what the chip did with the tables. Everything target-specific
lives in a plugin, `debugger/<arch>/tools/cycles_<chip>.py`, named on the
command line with the recording and the channels file. The profile loop prints
every cycle without consulting the tables; where a matcher segments cycles into
instructions, it also prints the matcher's verdict, so one run checks the
table, the matcher and the chip together.

The profile loop holds the debug pin active for the run, so `--capture` can
check the firmware's view of each cycle against a logic analyser triggered on
it. Decode strobes with a minimum width and merge a strobe split by a
nanosecond blip: crosstalk produces both, and either reads as an extra cycle.
The pin drops as the trap's vector read ends, too close to place that cycle,
so leave it out of the comparison.

Sample each signal where it is stable, which is not always where the data is
(see the HD6309's R/`#W`). Compare only the kind of a dummy cycle: nothing
drives the bus then, and the bus holders show whatever was there before. A
flash clears the emulated memory, and the trap fill with it, so fill again
before the next run.

Fix a table in its `.txt`, never in the `.cpp`: `inst_<chip>.awk` turns the
`.txt` into the matcher's arrays, and puts the command that did it on a
`// Generated by:` line before them. Run that command in the arch's directory
and replace what follows the line. The awk numbers the sequences in the order
the `.txt` first uses them, so a fixed row can renumber the rest; there is no
list in the awk to edit. `-v MODE=pretty` prints the `.txt` back aligned
instead, except in the Z280 and Z380 awks. **[code]**

### A frame that completes is not a frame that is right **[hw]**

`frames = 1` only proves a mandelbrot frame finished; check the output against
`samples/arith/mandelbrot.golden`. That comparison catches what a completed
frame hides: memory corruption on the bus, a wrong formula in a sample, and
division bugs in `arith.inc`. A mirror-symmetric but wrong frame points at
a sample's arithmetic; scattered wrong characters and restarts point at the
bus.

The `[arith]` expect block is matched as one substring, so every line has to be
what that sample prints, in order. Derive it from the sample's own `arith.asm`,
never copy it from another target.

### Bound a mandelbrot run by time, not by frames **[hw]**

Frame times range from seconds to many minutes. `lines = 6` halts at the first
completed frame, or once 6 rows are drawn and 20 seconds have passed, and
checks everything drawn against the golden frame.

`[gountil]` targets `loop_y` rather than `loop_x` on mid-speed CPUs (6 rows
within 20 seconds), so each go runs a whole row instead of one pixel.

To time a redrawing sample by hand, count the blank lines it prints between
frames: `bionic-control.py run <cap> <n>` stops after n of them and prints each
interval. A run that ends on its own has exited early and is a failure, not a
timing. **[hw]**

### Samples on another console device **[code]**

`io = "SCI"` in a sample's table routes the console through that device with
the `I` command for that sample only. Reset does not change the selection, so
the script reads the enabled device from `I`'s listing first and switches back
to it afterwards, pass or fail.

### Reset keeps the board's wiring **[code]**

A device's base address and interrupt vectors are the board's wiring, not
device state: a `Devs` constructor sets the defaults, then only `I dev addr
[rx [tx]]` or a program writing the vector registers changes them. `reset()`,
including the 8251's internal-reset command, clears only the device's own
registers and pending interrupts. Since R keeps whatever a run left
behind, the regress applies `[devices]` before each program it loads, and
`record-scenario.py` puts back any device a scenario moved.

A vector write moves a pending request to the new vector, as `I` does:
left on the old one, a vector of 0 can never negate it. Mandelbrot left
the 8085's Tx wired to RST 6, so echoir's command write raised it there,
its write of 0 to the Tx vector kept INTR high, and its `EI` took RST 6
into the stale handler.

What a command lists must read back through it as printed, since the tools
save the listing and restore it. `I` once read the base with `maxData()`'s
four digits while the i8096's listing printed six, so restoring `000400`
typed `0004` and set 000000, and every recording after that started with
the USART off. libcli caps a number's digits at its limit's, so a field
that echoes a listing takes the widest limit.

### Reset after uploading, not only before **[hw]**

`R` before an upload stops the CPU, but resets from the vector already in
memory -- the *previous* program's; uploading does not touch the PC. This shows
when samples start at different addresses (on the MC6800, `mc68xx` starts at
1000, the others at 0100): `G` then runs the stale program.
`bionic-regress.py`'s `load()` resets again after the upload.

For a CPU that fetches its PC from a reset vector, `[reset]` names the
vector's bytes, most significant first -- `PC = "[FFFE FFFF]"`, or
`"[FFFD FFFC]"` on the 6502 -- and the reset case checks the PC against the
value the loaded sample puts there.

### Some CPUs report the PC one byte early **[hw]**

The SC/MP (INS8060) and INS8070 increment PC *before* each fetch, so a stop at
`A` prints `PC=A-1`. Breakpoints and go-until targets still take `A`; only the
printed PC is offset. `pc_offset = -1` in a regress file's
`[breakpoint]`/`[gountil]` tells the script so. To start execution at `A` by
hand, set `PC=A-1`.

### Checking the analyser leads and board files **[hw]**

A preset's labels are only as good as the leads. Decode a reset capture before
trusting a new channel set: the register-save sequence after reset injects
known instruction bytes on reads and writes known register values, so swapped
strobes (such as crossed read and write strobes) show at once.

The `schematics/<board>/*_bionic.toml` pin maps are hand-written and have been
wrong (reversed data pins, wrong port, duplicated pin); check them against the
PCB's pad nets, not the other way round.

Capture a reset sequence with `R` once the prompt is ready: it runs the
same reset as a boot. Arm a capture triggered on the reset's negation
edge, then send `R`. A pulse-width trigger misses some resets: driving the
reset pin as an output glitches it first, and at power-up the reset is
already asserted, so the pulse is not always the width the firmware
holds. To capture a boot's own reset instead, remember that the firmware
waits at power-up until the console port opens, and that the daemon opens
it at each power-on to see what is seated. Hold the board lock
(`RUN_DIR/board-NAME.lock`, an flock) through the power cycle, arm once
the board is back on USB, then open the console. A capture armed before
the power cycle triggers on the power-off edges instead.

Keep captures triggered and short. `trimDataSeconds` only trims after the
trigger fires, so a stopped, untriggered capture holds the whole run, and
exporting one can fill the disk; closing the capture does not stop the export.

With the leads on the bench side of the connector, a line the CPU doesn't
drive keeps the last level the Teensy drove: an address bit that never
changes, or the previous read's data during ALE, points to an open line
between the CPU and the connector.

A stock footprint for a QFP's body size can be drawn for shorter leads. KiCad's
PQFP-100 14x20 mm ends its pads at a 17.2 x 23.2 mm span, while Intel's QFP
spans 17.9 x 23.9 mm with a 0.8 mm foot: the toes reached the pad ends, leaving
no fillet, and about 1 mm of each pad lay bare under the lead's shoulder. Size
a QFP's pads from the vendor's package table, from just behind the shortest
lead's heel to past the longest lead's tip.

### Order bench work by image, not by fix **[hw]**

Seating a chip takes moments; flashing the Teensy takes minutes and wipes the
emulated memory. Batch everything one image can test before flashing the next,
and when a fix needs a new image, first finish what the image on the board can
still do.

### Keep commit messages short

A subject line, and a body only when the reason isn't plain from the subject:
one or two sentences on what changed and why, never how the code got there.
Wrap the body at 72 columns.

### Record what a change taught in the same commit

Facts found while making a change — a bus timing, a pitfall, a bench lesson —
go into this document in that change's commit, not a later docs commit.

### Build with and without `ENABLE_SERIAL_HANDLER` **[code]**

The option is off by default, so `devs_*` files can declare a `SerialHandler`
under it without including `serial_handler.h` or the handler's own header and
nothing notices. Syntax-check a target with and without
`-DENABLE_SERIAL_HANDLER` when touching its devices.

### One ITCM overlay per target directory **[code]**

Only one target runs per power-up, so the `teensy41-overlay` image
(`-D ITCM_OVERLAY`) makes the code in the namespaces each `debugger/<dir>/`
declares an overlay: all of them are kept in flash and share one ITCM
address, and `Identity::instance()` copies in the one the identity EEPROM
names. RAM1 holds the shared code plus the largest overlay. Other images
keep every target in ITCM, but the rules below hold for all of them.

- Code that another directory calls belongs in the shared `debugger`
  namespace, as `DevsZ80` and `DevsMc6800` are; the link fails on a reference
  from one overlay into another.
- A target namespace has no global objects with constructors: startup would
  run them before any overlay is loaded. The build's overlay call check fails
  on such a call. Targets create their objects from `instance()`.
- Each `id_*.cpp` passes `TARGET_OVERLAY(<dir>)` to its `Identity`.
- `teensy_size` counts only the shared ITCM code; the largest `.ovl_*`
  section in `arm-none-eabi-size -A` adds to it.

## Per architecture

### Z280

**Pitfalls.**

- **Byte lanes follow the absolute address**, and a word read at an odd
  address is the aligned pair.
- **On-chip memory is a cache at reset**; injection is blind until
  `disableCache()` has run.
- **Refresh cannot be turned off**, only kept out of the ring.
- **`#NMI` must go out during the opcode fetch**, and in modes 0-2 it makes no
  acknowledge cycle.
- **The PC is logical, the bus physical.**
- **A captured sequence ends with a taken jump.**
- **Analyser leads on the multiplexed bus can stop it working at all**;
  unplug them before suspecting firmware. **[hw]**

#### Bus, lanes and sampling

**Z-BUS byte lanes.** The even-address byte rides AD8-15, the odd-address byte
AD0-7 (§13.5.1.1). The lane is chosen by the **absolute address**, never by a
buffer offset, in injection and in `MemsZ280::read_zbus`/`write_zbus` alike.
Uncached, the CPU issues a word read at every PC value, on both parities.
**[hw]**

**A word read at an odd address is the aligned pair**: the CPU takes the lanes
as bytes `addr & ~1` and `addr | 1`, and with the instruction cache on it
files both in the line. Answering with `[addr]`, `[addr+1]` works uncached but
caches a wrong byte before an odd jump target. The symptom: interrupt-driven
samples fail only with the cache on, and only at some link offsets. **[hw]**

**Sample the address inside the `#AS`-low window, the status after the rise.**
AD0-15 carries the address only while `#AS` is low. `ST0-3`, `R/#W`, `B/#W` and
`#AS` are 3-state and valid only from the `#AS` rise, which waits for #XTALI to
go low. A stopped PC outside the program, with a byte count that varies run to
run, means the address was sampled after the rise. **[hw]**

```
while (signal_as() != LOW) { ...clock... }
delayNanoseconds(addr_delay_ns);   // settle, still inside the #AS-low window
s->getAddr();
xtali_lo();                        // ...which makes the #AS rise
delayNanoseconds(status_delay_ns);
s->getControl();
```

**`completeCycle()` cannot count T states**: entered straight after
`resumeCycle()`, out of a `#WAIT` stretch, it cannot know the T state, so it
finds both edges by watching `#DS`; latching a write at a fixed T state breaks
every run. A later write sample without tracking state through
`resumeCycle()` remains open. **[hw]**

**No critical sections.** #XTALI is driven from `prepareCycle()`, so the bus
holds until the next edge; an interrupt only delays a sample. Under
`noInterrupts()` a CPU that stops issuing transactions takes USB and the halt
port down; unguarded, the watchdog catches it. **[hw][code]**

#### Clocking

`CLK = XTALI` with CS=01 in Bus Timing and Initialization (CS=00 gives
XTALI/2), ~16 ns XTALI→CLK. On a memory transaction `#AS` is low for one XTALI
phase; `#DS` falls one phase later on a read, two on a write, and stays low
three phases and two respectively. **[hw]**

**The CS latch takes effect only after the first bus transaction**; until then
`CLK` runs on a reset divider whatever CS says. `resetPins()` checks it past
`prepareCycle()`, where `#WAIT` parks the CPU in T2: sampled once per #XTALI
cycle, `CLK` is steady at CS=01 and alternates at CS=00. Earlier, extra cycles
carry the CPU past T1 and `prepareCycle()` misses `#AS`. **[hw]**

**Sustained clock rate is set by host code**: the high phase absorbs
`completeCycle()`, `_devs->loop()` and `prepareCycle()`, so trimming the delay
constants buys little. **[hw]**

#### Cache, refresh and the I/O page

**Cache Control** is control register `12`, reset `20` (M/C=0, I=0, D=1):
instruction caching on. `I=1` stops new fills but does **not** invalidate;
`PCACHE` (`ED 65`) drops the lines. Fixed-address mode (`M/C=1`) needs all 16
line tags initialised by data reads, which injection cannot carry (no `#M1`).
**[hw]**

**Refresh cannot be turned off**: §9.3 makes it a *minimum bus transaction
rate* — a refresh is generated whenever the timer elapses with no external
transaction, which at debugger speeds is always. Still set `E=0`, rate `3F`;
**rate 0 is not the slowest** (documented `0 < n < 63`, every 4n clocks, zero
special). Keep refresh out of the ring (see General). **[hw][doc]**

**The I/O Page register resets to 0.** On-chip peripherals live on pages
`FE`/`FF` and make no bus transaction; anything that changes the page must put
it back, or every later `OUT` stays on-chip and the debugger waits for ever.
**[hw][doc]**

**Control register addresses** (`LDCTL`, address in C) **[doc]**:

| Register | Addr | | Register | Addr |
|---|---|---|---|---|
| Master Status (MSR) | `00` | | Trap Control \* | `10` |
| Bus Timing and Control \* | `02` | | Cache Control \* | `12` |
| Stack Limit | `04` | | Local Address \* | `14` |
| Interrupt/Trap Vector Table Pointer | `06` | | Interrupt Status | `16` |
| I/O Page \* | `08` | | Bus Timing and Initialization \* | `FF` |

\* 8-bit; only the low byte of the source is written. `LDCTL HL,(C)` of an
8-bit register leaves H undefined. **[hw]**

#### Injected sequences and the resume origin

**Every sequence threads an `org`.** `execute()` resumes at `org`, keys its
window on it, and returns where it parked (its exit, or where the guard ran
out); sequences chain by passing the same variable. **[code]**

**The origin is a bus address; the PC is not.** The PC is 16-bit logical, the
bus 24-bit physical with 4K MMU pages. `RegsZ280` keeps the frame beside the PC
(`park(pc, addr)`); `nextIp()` and `physical(logical)` translate on that page,
identity off it. Exits are physical: `execInst(seq, len, org, physicalPc())`.
**[hw]**

**Capture the frame on a memory read**: after reset or a step's `RETN`,
`prepareCycle()` can return a refresh; `skipToRead()` clocks past it. **[hw]**

**End a sequence with `JR $+2`** when the host depends on its tail (a pending
`POP AF`, the NMI frame before `RETIL`, `restoreRegs`'s pop): the prefetch
passes the window before the last write lands, and the taken jump refetches
the exit after it. Each sequence declares its exit (`EXIT_END`, `EXIT_ORG`, or
its jump's target); the loop cannot infer it. **[hw]**

`RegsZ280::restore()` puts the frame below the SP it resumes with, so an
edited SP holds. **[code]**

#### Running: free-run, breaks and halts

**`loop()` free-runs** like `PinsZ80::loop()`, `_devs->loop()` each turn,
breakpoints patched as `RST 38H`; stepping it with `#NMI` would starve the
maskable interrupts. `setBreakInst()` patches one byte: the Z-BUS word
transfer lives only in `read_zbus`/`write_zbus`. **[hw]**

**A break must prove an `RST 38H` ran** (`_mems->read_byte(pc) == RST38`),
since a mode 1 interrupt also vectors to `0038H`. The `FFH` exit convention
holds only while the vector holds `FFH` (`ORG_INT` = `ORG_RST38` = `0038H`).
**[hw]**

**Find the push by scanning back**, not `prev()`: a prefetch may sit between
it and the vector fetch. In `suspend()` accept only the write right before the
`0066H` fetch or one read behind it; looser matches ordinary data. **[hw]**

**`#NMI` timing**: assert it **during** the stepped instruction's opcode fetch
— after `resumeCycle()` returns it, before `completeCycle()`. Later steps two
instructions; earlier steps none. `resumeCycle()` must return the opcode fetch
itself, not a prefetch. **[hw]** See the TMS9900 for the idiom.

**NMI in modes 0-2** pushes **PC only**, vectors to `0066H`, makes **no
acknowledge cycle** (`Signals::nmiAck()` is never true; the push is inferred
from its position), and returns with `RETN` (`ED 45`). The pushed PC is the
next instruction's, or a block instruction's own. To check for acknowledges,
probe all four status bits over the `#NMI` window; a `TSET` proves the whole
status bus in one cycle (`ST_LOCK` is `0xF`). **[hw][doc]**

**`suspend()` rules** (violations show as "halt, then continue sometimes does
nothing"): save only after it succeeds (else registers are read
inside the NMI service); every continue from a breakpoint steps over it
first, so a breakpoint hit twice then never is a stepper bug. **[hw]**

**`HALT`**: a prefetch can carry the CPU into a `HALT` past a breakpoint.
`rawStep()` refuses to step a `HALT`, and a halted run cannot be saved — no
boundary to inject at. **[hw][code]**

**`run()` saves before the dump**, holding the ring so the debugger's cycles
reuse the head slot, since the dump needs the stopped Cache Control. Each
`loop()` exit `Cycles::discard()`s its injection and returns whether the
registers can be saved. **[code]**

#### Interrupts and devices **[hw]**

- **The acknowledge vector rides AD0-7.** Mode 0 needs it; the samples build
  `db 3EH` + `rst 28H`, writing `A = 0EFH` to the UART's vector register.
- **Override `Devs::vector()`** (compare `DevsZ80::vector()`), or every
  acknowledge gets `00` and repeats for ever in mode 0.
- **Only an I/O transaction may reach a device**: gate selection on
  `s->ioReq()`. `intAck()` names the three maskable codes; `ST_NMIA` (`0x5`)
  sits inside `ST_INTAA..ST_INTAC` and must not get the USART's vector.

#### MMU and user mode **[hw]**

`samples/z280/mmu_echoir.asm` echoes in user mode (user page 0 at physical
0A5000H, the rest invalid); USART setup, the receive ISR and putchar/getchar
run in system mode via `SC` traps through the table at 2000H (mode 3,
`RETIL`). Constraints:

- The system stack, the vectors (0038H, 0066H, the table) and page 8
  (`CALL 8000H`) must be valid system pages; page 8 must map to itself.
- Breakpoints and the `RST 38H` exit are read at the *logical* PC, so work
  only in identity pages; a user program exits through a system call.
- User mode with interrupts needs mode 3: in modes 0-2 an interrupt clears U/S
  without saving the MSR (6.2.1).
- **A mode 3 NMI** is an acknowledge, `W(PC) W(MSR) W(identifier)` on the
  system stack, two table reads and the handler fetch. `suspend()` parks there
  in system mode; `save()`/`restore()` take and return PC and MSR via the
  frame (`LD SP,msr_slot; RETIL`). The handler never runs; its table entry
  must supply an MSR that disables interrupts; the system map is assumed
  identity.
- **MSR and I/O Page are registers**: dump line three shows `USP=`, `MSR=`,
  `IOP=`; `=MSR`/`=IOP` set them via `LDCTL` (C=00H, 08H). The MSR is written
  last, just before the jump.
- A 24-bit `org` emits extended-linear HEX records; the loader honours them
  until the upload ends.
- `ld HL, (IVT>>12)<<4` is an *indirect* load: a leading parenthesis is an
  address.
- Writing 0 to the emulated USART's vector register disables its interrupt,
  even in mode 3 where the byte is only the pushed identifier.

#### Programs with the cache enabled **[hw]**

`samples/z280/cmandel.asm` (via `enable_cache.inc`) turns on instruction and
data caching itself.

- Nothing may be injected over cached lines, so every stop parks at the
  *vector* fetch (`RST 38H`, 0066H or the mode 3 handler) and sequences run
  from there in system mode.
- A miss fetches one word, so injected code is fetched by even words; jump
  targets match by word and the CPU resumes at the word the bus showed.
- Entry purges, keeps HL and BC, reads Cache Control and sets 60H, in one
  window. Return runs from 8000H: restore Cache Control, BC, HL, purge, SP,
  return; what follows the purge stays cached, so it must not be the vector's
  line. A step leaves the cache off; the next save keeps the recorded value.
- A halt may be seen on an acknowledge with no address; the frame comes from
  the last memory read.
- Dump line three ends `CACHE=` (`__`, `I_`, `_D`, `ID`); `=IC 1`/`=DC 1` (0
  off) edit it, so `mandelbrot.hex` runs cached unmodified. Restore writes the
  MSR after Cache Control (except after a mode 3 NMI). After a modes 0-2 NMI
  the interrupt enables sit in the Interrupt Shadow register, read back only
  by `RETN`: dumped as `??`, not settable by `=MSR`.
- ST `1000` (cacheable) is the page attribute, on every memory cycle, so it
  does not show the cache state. With the instruction cache on the dump stays
  raw.

#### Single-Step and Breakpoint-on-Halt traps — not adopted **[doc]**

- **MSR bit 8 = SS**, **bit 9 = SSP**; each instruction checks SSP then copies
  SS into it, so setting SS (e.g. `LDCTL`) traps one instruction later. It
  pushes **PC and MSR**, the PC being the *next* instruction's (Division,
  Access Violation, Privileged Instruction and Breakpoint-on-Halt save their
  own); `RETIL` returns; re-executing traps clear SSP in the pushed MSR.
- **Breakpoint-on-Halt** replaces the first byte with `HALT`; enabled in Trap
  Control (`10`), vector at IVT offset `40h`.
- **All** traps use the vector table in every mode: it must sit on a **4K
  boundary** with pointer `06` set first. Single-Step is at **`3Ch`**. Any
  patch needs a `PCACHE`.

| | NMI (implemented) | Single-Step trap |
|---|---|---|
| Setup in target memory | none — fixed `0066H`, handler injected | 4K-aligned IVT + pointer register |
| Saved status | PC only (modes 0-2) | PC + MSR |
| Return | `RETN` | `RETIL` |
| Timing sensitivity | must assert during the opcode fetch | none — fires by design |
| Block instructions | can interrupt mid-block; saved PC is the block instruction | steps over the whole block |

#### Bus-cycle tables from the chip **[hw]**

The Z-BUS cannot tell a fetch from a data read and prefetch runs ahead, so the
tables are recorded. `scripts/record-cycles.py` with `tools/cycles_z280.py`
runs every pattern of `tools/gen_z280.lst.zst` and `tools/z280-opcodes.txt.zst`
in isolation over memory filled with `FF` (`RST 38H`); `tools/derive_tables.py`
writes `z280-PAGExx.txt` and `inst_z280.awk` the tables, hand-maintained from
then on. The raw recording, `tools/z280-profile.jsonl.zst`, is zstd JSON lines
via Python 3.14's `compression.zstd`. Recording needs the profile image; the
plain build prints `R A=xxxxxx D=xxxx` like the Z80's.

- **Prefetch**: at most 3 words before an instruction's last data cycle, 1-3
  at the exit; patterns are stable and independent of addresses and data.
- **A byte load at an even address is a word read**, at odd a byte read; an
  odd word is two byte transactions.
- **Multiply/divide** re-read the next word up to five times; EI, DI, PCACHE,
  LDCTL and others re-fetch after a flush.
- **Traps** are `WWRRS`; `derive` drops that tail from divide rows (the
  harness's operands trap), `SC` keeps it. **RETIL** is `RRA`, no `out(1)`
  despite Table E-1.
- Block instructions appear as `{...}` from one and two iterations, the second
  at the other parity.
- The relative and 16-bit indexed `FD ED` forms and register variants the
  pattern list omits are run as extra patterns; rows still missing are
  filled by operand shape from a recorded opcode on the same page (IX and IY
  pages mirror).

#### Disassembling the ring **[hw]**

`disassembleCycles()` uses `MatchWalker` with `InstZ280`: an instruction's own
bytes, then its data transfers, with prefetch and re-fetches absorbed unmarked
as the next instruction's fetch (the i8096 model). A taken transfer ends at the
target's fetch; an interrupt instead of the owed fetch matches as one.

Any start in a byte stream decodes plausibly, so the walk relies on the chain:
each instruction must lead to the next address, its target, the popped return,
a vector, or the stop PC. It backtracks until the chain holds; `findFetch()`
takes the earliest start ending at the PC. Cycles are marked only once a walk
succeeds.

`tools/check_samples.py` checks dumps against listings;
`test/z280/test_inst_z280` (`pio test -e native`) replays captured dumps and
checks marks, and with `Z280_DUMP=<file>` replays any dump, under sanitizers
if wanted.

#### Samples **[hw]**

`echo`, `arith`, `echoir` (mode 1), `echoitr` (mode 0) and `mandelbrot` run.
The four that exit do so at their own `rst 38h`; `mandelbrot` loops until
Ctrl-Space or any byte on the halt port, and a run that ends by itself has
failed. Frames are timed between blank lines (`docs/mandelbrot.md`).

### Z380

**Status: runs on the board**, all features including resumes with every
register bank; backtraces walk the fetch queue (below). **[hw]**

**Pitfalls.**

- **`#M1` marks no opcode fetch**, only an `#INT0` acknowledge and each
  half of a RETI; code is fetched with `#MRD` alone. **[doc]**
- **Code comes through a fetch queue, as on the 8086**: aligned words up to
  5 bytes ahead, a byte at an odd branch target (and a 7-byte
  instruction's last two). A word can hold two instructions and data cycles
  land among later fetches: no per-opcode bus sequence exists. **[hw]**
- **After an I/O write an `RST` fetches its vector before it pushes**, so
  `isRst38Break()` takes the first push. A break reported one push late,
  at the wrong PC, is the sign of looking back for the push. **[hw]**
- **In Native mode SP wraps within 64K**, SPz unchanged: rebuild it from a
  bus address with 16-bit arithmetic, or reset's SP=0 becomes 00010000H.
  **[doc]**
- **A push is one word in Native mode, two in Extended**, and the program
  may have run `SETC XM`: decide after reading the SR. **[code]**
- **Fetches are taken to be aligned words**: a sequence that runs off its
  end starts at an even address and has an even length. **[code]**
- **Reset does not clear SP, I or the bank 0 extensions**, despite Table 7;
  `RegsZ380::reset()` applies Table 7 after the first save. **[hw]**
- **`(HL)` uses all 32 bits of HL in Native mode.** `MULTW`, `DIVUW` and
  `SWAP` leave HL(31-16) set and a 16-bit `LD HL,nn` keeps it; a pointer
  in HL that misses by a multiple of 64K is the symptom. Samples address
  memory directly or through IX, and ISRs use `EXALL`. **[hw]**
- **An instruction not fully fetched when `#NMI` is asserted may not
  start**: a step that pushes its own PC shows it, typically at an odd
  address. When no register changed, `step()` asserts `#NMI` one or two
  cycles later. **[hw]**
- **A sequence counts only when finished.** If an NMI, a trap or a parked
  CPU cuts it short, `execute()` sets `_cutShort` and reset reports
  `?reset: setup sequence cut short` instead of saving shifted registers.
  **[hw]**

#### Board **[doc][hw]**

A0-A31 share P30-P37 through four 74HCS153; ASEL1:ASEL0 (mux B:A) select
A0-A3/A16-A19 at 00, A4-A7/A20-A23 at 01, A12-A15/A28-A31 at 10 and
A8-A11/A24-A27 at 11. Read them in gray order 00, 01, 11, 10 and back to
00, where the selects rest: the first nibble needs no settle delay, each
later step 15 ns (the mux follows a select within one 8 ns analyzer
sample). AL and AH are both on GPIO7, one read per step. **[hw]**
The KiCad netlist is authoritative: ASEL0 on P44, ASEL1 on P45, `#M1`,
`#MRD`, `#MWR` on P40-P42.

JP1 is open, so `#MSIZE` is high: word-wide memory, the even byte on
D8-D15 and the odd on D0-D7, as on the Z280. CLKSEL is high (CLKI is the
direct clock); `#BREQ` and `#INT1`-`#INT3` are tied high. Not connected:
BUSCLK, IOCLK, `#IORQ`, `#INTAK`, `#HALT`, the chip selects and `#TREFx`;
a halt is inferred from 1024 clocks without a strobe.

The first board carries a bodge wire for A11 (U1.95 to U4.13). Find
contact and solder faults from the bus, then fix them by reseating the
part or reflowing the joint of the line it names: the profile image's
reset walks a 1 and a 0 through each data lane and prints `?data` for a
bit that fails; a bad address line shows by storing a marker at
single-bit addresses and seeing where it lands. **[hw]**

#### Bus and clocking **[doc]**

BUSCLK is CLKI inverted, rising up to 30 ns after CLKI falls and falling
up to 27 ns after it rises; outputs follow BUSCLK by 6.5 ns. CLKI rests low
and `clki_cycle()` is high-then-low, sampling 40 ns after the fall. Sample
strobes with BUSCLK high (negated in T1, asserted in T3); sampled low,
they read asserted in both T2 and T4, and merged back-to-back
transactions are the symptom.

A memory transaction is T1-T4, two BUSCLK cycles. `#MRD`/`#MWR` assert at
the end of T1 and negate at the end of T4, where read data is latched; a
write drives data from T1. `#WAIT`, sampled at each boundary, inserts
whole BUSCLK cycles with the strobes held, so the CPU parks in T3 mid-run
and T1 at reset, and `completeCycle()` watches the strobes, as on the Z280.

`#IORD`/`#IOWR` span the second IOCLK rise to the fourth fall, where read
data is latched: hold data until the strobe negates. An `#INT0` acknowledge
holds `#M1` five cycles and latches the vector in the last. In a RETI the
CPU drives the data and `#IORD` joins `#M1` 1.5 IOCLK later, so `#M1`
alone waits up to 6 BUSCLK for `#IORD` before it is answered as an
acknowledge; answering early drives the vector against the CPU. Refresh,
on-chip I/O and a halt strobe nothing the board sees.

Reset leaves 7+3+7 wait states on memory below 1 MB and in the top one, 7
on I/O, and IOCLK at BUSCLK/8. `setupBus()` clears the wait registers
(on-chip 08H-0EH) and writes 02H to IOCR0 (11H): IOCLK = BUSCLK/2, so
`#IORD` joins a RETI 3 BUSCLK in, well before an acknowledge latches its
vector 10 cycles in. Not divide-by-1 (07H): the vector is then latched 5 BUSCLK
after `#M1`, before the wait can tell an acknowledge from a RETI. Every
mode 0 interrupt taking FFH (`RST 38H`) is the symptom of a vector latched
before the board drove it. **[hw]**

`resetPins()` insists on a read at 00000000H as the first cycle, else
prints `?reset:` with the cycle and saves nothing; a set address nibble
there points at the mux or its settle delay.

#### Memory and devices **[code]**

Main memory is the 16 MB of EXTMEM, decoded from A0-A23 and mirrored over
4 GB. The ring and register dump keep 32-bit addresses; memory commands
take 24. The USART is the Z80's at 140H (`USART_BASE` in `devs_z380.h`),
decoded from all 32 bits, byte I/O on D0-D7. `IN A,(n)` puts A on A8-A15,
so samples use `INA`/`OUTA`.

#### Registers and modes **[doc][code]**

`LDCTL` moves 16 bits in Word mode and never loads the mode bits; both
directions need `DDIR LW`. `save()` cannot know the LW mode, so it pushes
HL and the SR in 16-bit halves under `DDIR W`, then runs `RESC LW`. Upper
halves come down with `SWAP` (no flags touched); 32-bit values go back
with `DDIR IW,LW` immediate loads, AF by a real `POP` of a word staged
below the stack, as on the Z280. The SR is read with `DDIR LW; LDCTL
HL,SR`.

Of the mode bits, LW returns via `SETC LW` and XM, once set with `=XM 1`,
via `SETC XM`; XM clears only at reset. IM, IEF1 and LCK are never touched
(`RETN` restores IEF1 after an NMI), so the `SR` command edits the
selections and LW only. `r` shows the SR one character per field, `_` when
clear: `SR=Y3'X0_M1'XLI3L'` is IYBANK 3 with IY', IXBANK 0, MAINBANK 1
with alternate BC/DE/HL, then XM, LW, IEF1, IM 3, LCK and AFP.

All four banks are saved and restored, each register by physical side:
`LDCTL DSR,n` (`ED DA n`) picks bank and side for AF, BC, DE, HL;
`LDCTL XSR,n`/`YSR,n` (`DD DA n`, `FD DA n`) for IX, IY; AF' via
`EX AF,AF'` twice. `r`/`=` act on the set the SR selects; reach another by
setting `MB`, `XB`, `YB`, `IXP`, `IYP`, or `EX`/`EXX` (AFP, ALT) first;
`XM` and `LW` set the modes. `restore()` writes the selections last with
the byte forms of `LDCTL` (and `EX AF,AF'`), never `LDCTL SR,HL`.

`execute()` runs until it has captured the requested byte count, so the
count must be exact or it runs to the guard. An exit address counts only
after the sequence's last byte is fetched, since a restored PC may lie in
the window. `step()` refuses a `HALT` before `restore()`, since at an odd
restored PC no sequence can be injected.

#### Instruction boundaries **[hw][code]**

The walk (`MatchWalker`, `InstZ380` as Arch) follows the code stream: each
instruction's bytes are fetched, then its data cycles and any target fetch
come in any order amid sequential fetches. An interrupt, NMI or trap is a
PC push and vector fetch, in either order, between instructions or in
place of a pending target fetch. The walk must end at the stop PC (perhaps
already queued); the earliest start wins, conditionals tried taken first,
and instructions before the first with data or a transfer are dropped. The
backtrace prints each instruction, then its data cycles.

The tables put data and transfer in an unordered `[…]`, each kind's bytes
in order, with Long Word and Extended variants where they differ and `a`
bytes a `DDIR` immediate widens; `DDIR W`/`LW` pick the variant. An
instruction's reads, and its writes, must be contiguous, which stops a
`RET` "popping" a stray fetch.

`tools/derive_tables.py` writes the tables from the recording (every
libasm opcode and `DDIR` form, odd and even operands, both condition
outcomes, Extended and Long Word passes) in `debugger/match_legend.md`'s
tokens. `tools/rings_z380.py` writes the host test's rings and the bench
rings in `z380-rings.json.zst`, checked against the sample listings.
`tools/README.md` has the recording's pitfalls.

#### Samples **[code]**

`samples/z380` are the `samples/z80` programs rewritten: arithmetic in
`MULTW`, `DIVUW` (`divu16`, unsigned; `div16` signed on it), `SUBW`, `NEGW`,
`CPW`; I/O by `INA`/`OUTA`; queues through IX; ISRs on the alternate
registers. Keep routine entries and workspace word aligned: odd-address
fetches and word accesses cost an extra bus cycle.

#### To check on the bench

- An interrupt inside an injected sequence after an RST 38H break with
  interrupts enabled, and a PC pushed as two bytes at an odd SP; the Z280
  driver shares both.

### Z80 / Z180

**Pitfalls.** Sequential-cursor injection: its faked-POP idiom must not be
copied to a word-bus target. I/O addresses are 8-bit but on the Z180, so
high-byte bugs stay hidden there.

Byte bus, one byte per read, sequential-cursor injection, so the faked-POP
idiom (`POP rr` followed inline by its payload) works and is used throughout
`regs_z80.cpp`. **[code]**

The Z80, NSC800 and KL5C80 `completeCycle()` treat an I/O address as 8-bit and
ignore the high byte; the Z180's decodes all 16 bits, its USART being at
0140H. **[code]**

### TMS9900 family

**Pitfalls.** Assert the halt interrupt only after the opcode fetch, or the
step lands on the wrong instruction.

`pins_tms9900_base.cpp::suspend()` is the reference for that idiom: it walks
the loop with a `bool assert_nmi` latch and asserts on the first cycle where
`s->fetch()` is true. Targets without an `#M1`-equivalent use "the
transaction `resumeCycle()` handed back", which is the opcode fetch by
construction: every sequence parks on the read that follows it. **[code]**

The TMS9980, TMS9995, TMS99105 and TMS99110 also run TMS9900 code. **[hw]**

The TMS99105 board names its CPU by behaviour, not by marking: at reset an
injected `CIR` makes two writes when a floating-point macrostore answers.
The bench's chip, marked TMS99105, has the TMS99110's ROM on chip, so
`MACRO=STANDARD` reports TMS99110 and only `MACRO=BASELINE` runs it as a
plain TMS99105. The `?` banner reads `BionicTMS99105 (CPU: TMS99110)`;
take the CPU from the `CPU:` field, not the board name. **[hw]**

#### TMS9980 bus timing

**CKIN never stops.** CKIN must stay within 6-10 MHz, and a bit-banged
clock held at every cycle boundary by the work between cycles broke
TMS9980s from run to run. CKIN is FlexPWM2 submodule 1 on pin 5 at
10 MHz, and `READY`, low but to end a memory cycle, holds the CPU in each
one until it is served. The TMS9981 shares this code. **[hw][code]**

**Sample at the `#PHI3` fall.** A cycle's address, control and write data
are valid some 100 ns before it. Non-memory clocks are not held, so a
read of the bus at any other moment, `resumeCycle()`'s too, may catch the
next cycle's address changing. **[hw][code]**

**`READY` is sampled at each `#PHI3` rise of a memory cycle but its
first, with tsu tw-30 and th 2tw+10** (Data Book Figure 9b and switching
characteristics); the rise that sees it high latches read data. It is
raised 250-290 ns after a rise, between that rise's hold and the next
one's setup, and dropped after the latching rise's hold, two clocks
before the next cycle samples it. A memory byte takes three clocks or
more. **[hw][code]**

### 6502

**Pitfalls.** Power-cycle after swapping chips on the 6502 board: `R` does
not redetect the part, so the debugger keeps the previous chip's identity
and instruction set: a wrong CPU name after a swap means no power cycle.
**[hw]** `Regs` needing memory gets `Mems*` through its
constructor, and some targets build `_regs` before `_mems`.

`RegsMos6502` takes its `Mems*` as a constructor argument rather than reaching
through `Pins`, because `Pins::_mems` is protected and only `Target` is a
friend. Follow that pattern when a `Regs` needs memory, swapping the
construction order first where `_regs` comes first. **[code]**

### F3850

**Pitfalls.** A Teensy interrupt mid-cycle desynchronises the ROMC sequence;
mask them around each bus cycle.

Mask Teensy interrupts around each bus cycle in `cycle()`; a lost sync shows
as a stack pointer off by one and nondeterministic results. **[hw]**

Release the data bus right after `Cycles::next()`; the bus level holder keeps
the value for the CPU, so the turnaround needs no wait. **[hw]**

The 16-bit compares in `arith.inc` compute `m + ~s + 1` and derive the flags
from that. **[hw]**

The profile (`tools/cycles_f3850.py`, recorded as
`tools/f3850-cycles.jsonl.zst`) matches every opcode to the data sheet's
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

**Samples fit in 64 bytes of internal RAM**, the smallest of the parts the
board takes: past the registers and stack, 20h-3Fh is all there is. The
MSM80C35 wraps 40h-7Fh onto 00h-3Fh, so a variable at 40h overwrites R0:
a sample that is right on the larger parts and wrong on it points there. It
also reports as MSM80C39, since `DEC @R0` tells only the maker apart. **[hw]**

The fetch data drive is released about 300 ns before `#PSEN` rises. It is
harmless on the parts tried. **[hw]**

The profiles (`tools/cycles_i8048.py` on the P8039 and
`tools/cycles_msm80c39.py` on the MSM80C39, recorded as
`tools/i8048-cycles.jsonl.zst` and `tools/msm80c39-cycles.jsonl.zst`) show
machine cycles that pulse only ALE, with no strobe. **[hw]** `i8048.txt`'s `s`
column gives the cycles that do strobe, as Intel/OKI where the makers differ:

- `IN A,P1`, `IN A,P2`, `OUTL P1,A`, `OUTL P2,A` (09, 0A, 39, 3A) and the
  MSM80C39's `MOV P1,@R3` (F3) strobe once in their two cycles.
- `RET` and `RETR` (83, 93) strobe twice on the Intel parts, once on the
  OKI ones.

Counting a strobe that never comes makes the matcher take the next fetch for
the instruction's own cycle; on the MSM80C39 that hands the CPU a real `HALT`
and wedges the board.

The interrupt-driven samples' ISRs switch to register bank 1 and call the
queue routines, which push through R1 onto the software stack in external
data memory. A reset leaves bank 1's R1 as it was, so the ISR has its own
stack, at `isr_stack`; otherwise it overwrites the main program's stack.
**[hw]** Wrong results only where computation overlaps serial output (such
as the pixel computed while a newline goes out) point to an ISR corrupting
the main program's state.

### MCS-51

**Pitfalls.** An 80C51 in idle mode makes no bus cycles, and a reset in the
middle of a run must be followed by `restore()` and must end the run.

A P80C51 in idle mode holds ALE high; `prepareCycle()` gives up after
`idle_cycles` and resets the CPU rather than wait for ever. `resetPins()`
saves the registers by running injected code at 0000, which leaves the PC
where that code ended, so it needs a `restore()` (`LJMP` to the saved 0000)
or the CPU runs on into whatever memory follows, which looks like leftover
code running past the injected sequence. **[hw]**

The reset comes in the middle of `rawStep()`'s instruction, so the run
stops there: carrying on counts the old instruction's cycles against the
new program, loses the instruction boundary, and the next halt fails
("?halt: no writes"). **[hw]**

`udiv16_8` in `arith.inc` keeps the ninth bit when shifting the partial
remainder: when the shift sets carry, it subtracts unconditionally. **[hw]**
Test division with a divisor of 128 or more and a dividend above 255 to
exercise that bit.

An interrupt latched at a halt makes its LCALL at the first injected fetch,
and the save's bytes run as its handler. `park()` feeds `SJMP $` to fetches
at the halt address and the next until it has run twice without one, or a
fetch at a vector shows the LCALL taken and the save starts at the handler.
The LCALL pushes to internal RAM, so the vector fetch is all the bus shows,
and `SJMP $` makes a dummy fetch two bytes on, which can be a vector address
itself (002B). **[hw]**

R0-R7 are internal RAM 00h-1Fh, four banks picked by PSW's RS1:RS0; dumping
00-1F shows all of them. **[doc]**

### 8080

**Pitfalls.** Sampling SYNC or status before the datasheet's delays after
PHI2 rises reads a released bus and writes stale data over code.

`prepareCycle()` must not sample SYNC before tDC (150 ns max) or status before
tDD (220 ns max) after PHI2 rises. Sampling SYNC early misses T1; the status
is then read in T2 from a released bus, some reads look like writes, and
stale bus data is written over code. **[hw]**

Compare memory with the loaded image after a run to tell bus errors from
program bugs: bus corruption hits operands the CPU has been reading. **[hw]**

The NMOS 8080 is dynamic (tCY max 2 us). To rule clock stretching in or
out, arm a pulse-width trigger on PHI1 during a failing run; it has never
fired, so stretching is not a cause of such corruption. **[hw]**

PHI1 and PHI2 run free from FlexPWM2 and FlexPWM3. An XBAR sync from one
to the other's EXT_SYNC never started the second counter, so the two
are restarted back to back with FORCE, and PHI2's edges are set 4 counts
early for the gap between the writes. They reach the CPU through passive
level shifters: a rise takes about 100 ns, PHI1 peaks near 8.4 V, and
only fixed phases as wide as the bit-banged ones (PHI1 100 ns, PHI2 at
least 227 ns) keep it working, which tops the clock at 2.5 MHz. **[hw]**

- READY must change outside the window where PHI2's fall samples it:
  raised there, it missed tRS, and the CPU went on with part of its
  state waiting, reading memory without fetches until it ran into the
  weeds. `raise_ready()` waits on PHI1's counter.
- The status is on D0-D7 only around SYNC, so the cycle the CPU waits in
  carries it in its ring slot; `Cycles::reset()` rewinds to a stale slot,
  and resuming from it served a fetch as a write, writing the held bus
  over code. `resetCycles()` moves the waiting cycle along.
- In a run, a plain memory read is served without a wait state: READY
  goes high again as soon as the status shows it, the data is staged
  with `busWriteFast()` and driven the moment DBIN rises. A fetch is
  held every 10 us to run the devices, which change INT only while the
  CPU waits, and to poll the halt switch. **[hw][code]**

### 8085

**Pitfalls.** RST 5.5/6.5/7.5 make no INTA cycle.

RST 5.5/6.5/7.5 are vectored inside the CPU: after a discarded opcode fetch it
pushes the return address and jumps to the fixed vector, with no INTA cycle.
**[hw]**

X1 runs free from the PWM at 150 MHz / 15, the 8085AH-2's rated 10 MHz.
READY goes low as ALE rises and high once the cycle is served, and stays
high until the next ALE: DAD's two bus idle cycles have no ALE, so they
must never wait. AD0-AD7 carry the low address only until tLA after ALE
falls, so it is latched there, with interrupts masked from raising READY
to the next ALE. The acknowledge of TRAP or RST 5.5-7.5 has no strobe
and does not wait, so a step's TRAP is the cycle before its pushes, and an
`OUT` just before it makes three writes in a row. **[doc][hw]**

`sio.asm` times its SID/SOD bits as 312.5 CLK, but READY holds each of
its bus cycles, so on the serial handler's image at 10 MHz X1 they
measure 103-105 us, about 520 CLK: the handler's 80 x 13 X1 per bit was
taken from a capture of SOD and SID, not from the listing. **[hw]**

### 8096

**Pitfalls.** A missing `#` silently assembles the direct form; INT_MASK bit
n is the vector at 2000H + 2n; a word access needs an even address.

`ldb INT_MASK, INT_EXTINT` without `#` assembles to the direct form and loads
register 40H, so the interrupt works only when that register happens to
hold the right bit. An interrupt that comes and goes with unrelated register
contents points to such a slip; the listing's opcode (`B0` direct vs `B1`
immediate) shows it. **[code]** INT_MASK follows the vector table, bit n
for the vector at 2000H + 2n, so EXTINT at 200EH is bit 7, `10000000B`; bit 6
is the serial port's interrupt. **[hw]**

The profile (`tools/cycles_i8096.py`, recorded as
`tools/i8096-cycles.jsonl.zst`) checks the cycle rows. **[hw]**

- `PUSH [w]`, `PUSH n[w]` and `POP n[w]` prefetch once more between the
  operand read and the writes: `PUSH [w]` is `1:2:~:R:r:~:W:w`. `POP [w]`
  makes no such read.
- `DIV l,[w]` (`FE 8E`) prefetches before its operand read, as the other
  indirect forms do.
- When the short-index form of a `/` row fails, `InstI8096::match()` stops at
  the `/`: going on to an empty sequence matches without taking a cycle and
  `matchAll()` loops on it for ever, wedging the board on a backtrace.
- A jump taken to a word the queue has fetched already reads it again; the
  walk takes that fetch as the jump's (`targetAhead`), on the 8096 as on
  an 80C196. A `JLT +2` over a `CLRB` fetches 220EH either way; the store
  after it, of the register the `CLRB` would clear, shows it was taken.
  **[hw][code]**

A word access needs an even address; the profile keeps `n + [w]` even.
The pointer register's LSB selects auto-increment for `[w]` and the long
index for `n[w]`, and the profile uses an even register, so the long-index
forms are not checked on the chip yet.

**The bus is 16 bits wide.** The debugger serves the CCB as FFH, whatever
memory holds at 2018H. Every read is then a word's at an even address and
the CPU discards the byte it doesn't need (8X9X User's Manual §7.6)
**[doc]**, but a read that wants only the odd byte puts the odd address on
the bus: a fetch at an odd jump target, `LDB` from an odd address. Answer
both lanes from the even word whatever A0 says. **[hw]**

- The CCB is read before the CCR selects the bus width, and the manual
  says only that the chip "will correctly read this location in every bus
  mode". Driving it on AD0-AD7 alone works. **[doc][hw]**
- #BHE is bonded out on the DIP-48, despite the data sheet's note that it
  isn't on 48-pin parts: `PUSHF` pushes in one word cycle. **[hw]**
- A status read at 0401H would read the data register at 0400H with it
  and swallow a character, so the USART's registers sit a word apart:
  data 0400H, status and control 0402H. 0400H is above the 80C196NP's
  register RAM, so every chip runs the same samples. **[code]**
- Injection is address-keyed: each sequence runs from where the CPU is
  parked and jumps back there, and the read of the exit is held for the
  next one. The vectors the debugger hands a trap are even, so a sequence
  never starts in the middle of a word. **[code]**
- The queue reads a word again: twice running, and after one it had too
  little room for, sometimes past a write. A ring may also start up to
  seven bytes into the instructions it cuts. `refetchWord` and `cutStart`
  in `ArchI8096` say so; the bench rings `bench_i8096_16` hold the cases.
  **[hw]**
- The queue fetches a word ahead before a 3-operand `n[w]` form's read,
  and before the long-index read of `MULU`, `MULUB`, `MUL` and `MULB`. The
  profile on the 16-bit bus, `tools/i8096-cycles16.jsonl.zst`, checks every
  row. An opcode and its TRAP may come in one word, so `check()` takes one
  marked fetch for a one-byte instruction. **[hw]**
- A word fetch may bring an instruction's first byte at its odd address,
  or two instructions' first bytes: the walker's `markStart()` tells the
  backtrace which, or it disassembles the byte before. **[hw][code]**

**INT_PENDING is undefined after a reset**, as SP is (8X9X User's Manual
§13.2). **[doc]** An EXTINT latched at power-up, or while the Teensy
reboots after a flash, survives every reset and is taken as soon as a
program enables it, so the debugger's reset clears INT_PENDING. **[hw]**
A step that sticks once after a power cycle and never again points to state
a reset leaves undefined.

**N8097BH.** The PLCC-68 part runs the P8095BH's code; its board has INST
on P46 and NMI on P50 where the P8095BH's has PWM and ACH4, and leaves
BUSWIDTH open, which floats high. **[doc]** INST is high on every
instruction fetch, prefetches included, and low on data: the CCB read and
every write. **[hw]** The debugger keeps NMI low but to halt, since a rise
vectors to 0000H, and reads INST as the fifth CNTL line. **[code]**

- An open address line (AD9, CPU pin 51, on the first board) follows the
  last level the Teensy drove: a fetch at 2082 reads as 2282. Compare the
  addresses the bus shows with the expected ones (the reset vector, the
  value loaded into SP) to find the one bit that is off; if reseating
  doesn't help, check that pin's joint before suspecting firmware. **[hw]**

**S80296SA.** The QFP-100 board serves CCB0 C6H and CCB1 DAH (multiplexed,
16-bit, 64-Kbyte mode) and sets BUSCON5 to 40H for everything outside
F2000-F20FFH. **[code]**

- It reads a vector before it pushes the PC, as the 8096 does, not first
  as the 80C196KB and KC. **[hw]**
- Its reset, an RST's too, holds ALE high before the CCB0 fetch's own
  ALE, so a cycle's address is the last ALE's before its strobe. A
  program's RST resets BUSCON5, so the debugger sets it again, reading out
  the register that takes first. **[hw][code]**
- It can take an interrupt inside a call, after the call pushed its return
  address: it pushes that address again, and the handler returns past the
  callee, leaving the word on the stack. The bus shows the push, the vector
  read, then the same push; its Specification Update (A0) doesn't list it.
  With XTAL1 high for 12-265 ns, low up to 1 µs longer, or EXTINT changed at
  either CLKOUT level, it dropped as many rows of mandelbrot: the chip's
  logic, not the clock's margin. The interrupt-driven samples disable
  interrupts before each call made with them enabled, and the callee enables
  them again. The debugger's halt is an NMI, which no DI holds off: when the
  cycle before its vector read pushes the halted PC at the interrupted SP,
  the halt resumes at the call's target, the call's push left on the stack.
  **[hw][code]**
- An odd register operand selects another instruction: a signed MUL or
  DIV without the FE prefix, MVAC or MSAC, EBR; a MUL whose destination
  byte is below 10H is the MAC family. The profile gives the base forms an
  even register and records those forms on their own. **[doc][hw]**
- With its queue full the bus runs 12 bytes past the instruction, beyond
  the walker's old cap of 8. A ring a run began with keeps its first
  instruction through `walk()`'s `first`. **[hw][code]**
- EXTINT0 is P2.2's special function: a sample sets P2_MODE, P2_DIR and
  P2_REG, then clears the pending bit writing P2_MODE sets. **[doc][hw]**
- WSR1 windows 40H-7FH and PUSHA leaves it: the save stores it out and
  clears it, the restore loads it before POPA. **[doc][code]**
- AD lines open at the QFP's joints latch CCB0 with stray bits (DE for C6:
  demultiplexed, five wait states); reflowing the AD pins fixed it. **[hw]**

### SC/MP (INS8060)

**Pitfalls.** The printed PC is one byte early (see General); routine
addresses are `ADDR(label)`, one less than the label.

PC is incremented before each fetch, so it holds the address before the next
instruction. Routine addresses are loaded as `ADDR(label)`, and a call is
`XPPC P1` with the operands inline after it. **[doc]**

XIN runs free from the PWM at 150 MHz / 37, halves of 120 and 127 ns.
`#HOLD`, low but to end a cycle, stretches `#RDS`/`#WDS` until the cycle
is served. The status and A12-A15 are on DB only around `#ADS`, which
`#HOLD` does not stretch, so `#ENIN` grants each cycle with interrupts
masked until `#ADS`, and goes high only once `#ENOUT` shows the cycle
over: a cycle `#ENIN` cuts short is repeated, and an ILD or DLD's store
waits for it. The board ran to 6.25 MHz XIN, 80 ns halves, set by `K`.
**[doc][hw]**

`divsi2` in `arith.inc` must divide on every sign path, including a negative
divisor, and must take the quotient's sign from the sign bytes, never the
divisor's low byte. **[hw]**

### INS8070

**Pitfalls.** Same early PC as the SC/MP; reset leaves P2, P3, T, E and A
as they were; `DIV EA,T` leaves no remainder.

Same PC convention as the SC/MP. Reset clears only PC, SP and the status bits
other than SA/SB; P2, P3, T, E and A keep leftover values. **[hw]**

XIN runs free from the PWM at 150 MHz / 37, as on the SC/MP. `#HOLD`,
held low from before the strobe, keeps the address, data and strobe of
an external read or write until the cycle is served. The manual shows it
lowered after the strobe starts, but held low throughout it works on the
bench. `#ENIN` stays low through a run: the address is on its own lines
and held, so a cycle started while the Teensy is busy waits for the next
`prepareCycle()`. The board ran to 6.25 MHz XIN, set by `K`. **[doc][hw]**

INS8073 BASIC's bit-banged serial times its bits with loops whose
fetches the Teensy holds, so in XIN its bits depend on the loop: at
4800 bps it sends a bit every 1012 XIN and receives, echoing each bit,
one every 1078. The handler's dividers were measured on F1 and `sio.asm`
is tuned to them; a `DLY`-timed SC/MP loop needs no such tuning. **[hw]**

INS8073 BASIC starts its program at 8000H when it finds RAM there, as
the debugger's memory has, so its console patches end RAM at DEFFH, just
below the ACIA; ending it at 7FFFH answered every program line with
ERROR 1, out of memory. **[hw]**

An interrupt handler must save S: an interrupt between an `ADD` and the
`LD A,S` that reads its carry otherwise changes CY/L. The samples' ISRs
push it after EA and restore it before `OR S,=S_IE`. The bit-banged clock
hid this: devices raised interrupts only between bus cycles of a stopped
CPU, so each landed on the same instruction every time; on the
free-running clock they drift, and `echoitr` printed a wrong bit about
once in 50 characters. **[hw]**

`DIV EA,T` leaves the quotient in EA but not the remainder in T;
`print_uint16` multiplies back to get it. **[hw]**

`-vF` as a displacement is not "minus F" when `vF` is offset 0. A
mandelbrot frame that is symmetric but wrong points to a sign error like
this. **[hw]**

A backtrace that never matches past one opcode points to a missing cycle
sequence. `XOR A,d,PC` (`E0`) reads its operand PC-relative like the other
PC-relative forms, `1:2:Q:N`, as the profile (`tools/cycles_ins8070.py`,
recorded as `tools/ins8070-cycles.jsonl.zst`) shows. **[hw]**

### MC6800 family

**Pitfalls.** `WAI` stacks first and then idles, so a halt finds no push
(and on VMA parts no write count change); `#XIRQ` is level-sensitive; the
MC68HC11's `STOP` can bring E back at another phase.

Three boards run six chips: MC6800 (and MB8861), MC6802 (and MB8870), MC6801
(and HD6301). The first two share `samples/mc6800`; the MC6801 has its own
samples. The HD6301 and MC6801 also pass the MC6800 regress, running MC6800
code. **[code]**

`PinsMc6802` builds a `MemsMc6802`; never cast a base `Mems` object to a
derived type. **[code]**

`WAI` stacks the context up front and then idles, which a halt must allow
for; a board that wedges past the halt port when halted in `WAI` breaks one
of these: **[hw]**

- On the MC6800 and MC6802 the idle cycles have VMA low, which leaves
  `_writes` alone, so `loop()`'s context-save branch clears `_writes`, or it
  would be taken on every cycle and never poll the halt switch. A write count
  stuck at the push's 7 shows this.
- The halt's NMI then fetches its vector without pushing anything. A read of
  the NMI vector before the push means `WAI`: `suspend()` injects `RTI` to
  resume after the `WAI`, then takes a fresh NMI edge. The wait is bounded.
- Stepping a `WAI` puts the push first and the vector fetch an unknown number
  of idle cycles later, so `suspend()` looks for the vector read rather than
  a fixed cycle, and releases the line only after it: `#XIRQ` on the
  MC68HC11 is level-sensitive, unlike NMI.

The MC6801 has no VMA, so only the second applies to it.

The profile (`tools/cycles_mc6800.py` and its siblings, with a recording
for each of the six chips in `tools/`) shows `CPX #n16` taking four cycles
on the MC6801, `1:2:3:x:N` like `SUBD #n16`. **[hw]**

The MC68HC11's `STOP` freezes E, and after the wake-up E can come back at
another phase against the EXTAL the debugger drives; every later cycle is
then sampled at the wrong time, so garbage registers at a halt after `STOP`
point to E's phase. `rawCycle()` checks that
E is high where it must be; if not, it marks the cycle non-VMA and walks
EXTAL until E falls again. **[hw]**

### MC6805 family

**Pitfalls.**

- `WAIT` and `STOP` fetch nothing until an interrupt, so a halt must give up
  and take an IRQ; `STOP` restarts the bus clock at another phase.
- On the MC68HC08AZ0 the run loop follows each instruction's cycle sequence,
  so a wrong table row, or an interrupt taken over a prefetched opcode, loses
  the instruction boundary and with it halts and breakpoints.
- `MUL` and `DIV` re-read the next opcode's address: a step's `SWI` must go
  into every read of it.
- An interrupt's stacking can fall where the cycle table expects the last
  prefetch, the slot a halt injects its `SWI` at: no halt there on a write.
  **[hw]**
- The HC08's COP watchdog is a mask option and cannot be disabled; its reset
  also turns off the internal read visibility the debugger needs, so the run
  loop sets it up again at the reset's vector read.
- With IRV on, the MC68HC05C0 drives the bus in its internal cycles, and
  not every one has an internal address: the extended-indexed (`a16,X`)
  dummy read, the third cycle after a `Dx` fetch, reads `hi:FE`. Never drive
  the bus there (a capture of `D0` shows the fight); the debugger counts
  cycles from the fetch #LIR marks and reads that one. **[hw]**

Three boards run three chips: MC146805E2 (`samples/mc6805`), MC68HC05C0
(`samples/mc68hc05`) and MC68HC08AZ0 (`samples/mc68hc08`). The HC05 and HC08
also run MC146805E2 code. The HC08 has its own run loop; the other two share
`PinsMc6805`. **[code]**

On the MC146805E2 and MC68HC05C0, a halt in `WAIT` or `STOP` would wait
forever for an opcode fetch. **[hw]** `suspend()` gives up after 64 cycles
and takes an IRQ -- both clear the I mask -- whose five-byte push stands in
for the `SWI` that `save()` would inject; stepping over `STOP` goes the same
way. `WAIT` restarts the bus in phase, but `STOP` freezes the bus clock with
DS high or low and restarts it at another phase against the OSC1 the
debugger drives, so after `STOP` the MC146805E2 walks OSC1 through a whole DS
pulse first, as reset does. The loop records the last opcode fetched to tell
which it was: by the time of a halt the ring has lost it.

`captureExtra()` must inject the SWI vector with the dummy cycle that
follows it, as `save()` does; otherwise the CPU runs a cycle behind, the
next injected `LDA #` loses its opcode, and its operand, the saved CC, runs
as an instruction. Test it by stepping after a breakpoint inside an
interrupt handler, where that CC is an opcode such as `FD` (`JSR ,X`).
**[hw]**

Each sample binary has one ACIA address and runs as is on every chip that
can run its code. The MC6805 samples use the MC146805E2's ACIA at 17F8, and
`[devices]` in their HC05/HC08 regress file moves the ACIA there; only
mandelbrot runs on those two, the MC68HC05 and MC68HC08 having their own
echo and arith. **[code]**

On the MC68HC08AZ0, MORA (`$1F`) is read-only with COPD clear, so a program
that doesn't service the COP is reset every 2^18 OSC1 cycles. **[hw]** The
run loop redoes `reset()`'s setup at the vector read and jumps to the
program's vector. Sample variables start at `$50` or above: `$40` is not
RAM on it. Every sample that can run on the MC68HC08 writes COPCTL (`$FFFF`)
itself, unconditionally: on the MC68HC05C0 FFFF is external, and the
MC146805E2's 13-bit bus sees it as 1FFF, the reset vector's low byte,
written back unchanged. Loops that wait without reading the ACIA service
the COP themselves. `restore()` writes COPCTL before every run, as a
captured write that leaves the emulated reset vector alone, so each run
starts a full COP period. A sample that runs a line and then goes astray
on the HC08 is a sign of an unserviced COP.

The HC08's run loop follows each instruction's bus-cycle sequence from
`mc68hc08-P*.txt` to find the next opcode fetch, where a halt injects its
`SWI`. `scripts/record-cycles.py` with `tools/cycles_mc68hc08.py` records
every opcode on the chip, on the profile image; cross-check it with a
logic-analyser capture framed by the debug pin. The tables match it, and
`haltgo`, halting repeatedly, shows a wrong row as a wedged halt. **[hw]**

- `DIV` makes one next-opcode fetch.
- Where AN2627 gives an instruction more program fetches than bytes
  (`TXS`'s `pp`, `MUL`'s `ppddd`), the extra ones re-read the next opcode's
  address or the byte after it -- `TXS`'s second reads past the next opcode
  -- and the tables have them as dummy reads.
- `run()` completes the first opcode fetch, so the loop starts at the cycle
  after it.
- An interrupt taken over a prefetched opcode drops the prefetch and stacks
  five bytes. Five writes in a row inside any sequence but `SWI`'s mean an
  interrupt, and an opcode without a sequence halts with the cycles that led
  to it. When the loop stalls, decode the byte it took for an opcode against
  the opcode map: an ACIA status read taken for a fetch shows as an illegal
  opcode (`$82`), and a key press that turns it into `SWI` (`$83`) lets the
  program go on.
- `MUL` and `DIV` read the next opcode's address again after the prefetch
  and take the opcode from that read, so a step's `SWI` goes into every read
  of that address.

`WAIT` and `STOP` make one bus cycle, the next opcode's prefetch, and then
none until an interrupt or a reset. **[hw]** The loop tells that from eight
bus cycles of silence rather than from the opcode, keeps the devices running
while the CPU sleeps, and on a halt takes an IRQ whose push stands in for
the `SWI`, as the other two chips do. A reset ends a sleep, or a run, with
its vector read, and the loop realigns there: a program that sleeps without
a periodic interrupt is reset by the COP every 2^18 OSC1 cycles. Stepping
`WAIT` or `STOP` on the HC08 is refused. **[code]**

The profile (`tools/cycles_mc146805e2.py` and `tools/cycles_mc68hc05c0.py`,
recorded as `tools/mc146805e2-cycles.jsonl.zst` and
`tools/mc68hc05c0-cycles.jsonl.zst`) matches the tables on both chips.
**[hw]** Each pattern is padded with `SWI` to four bytes, longer than any
instruction, so a shorter pattern never leaves an earlier one's bytes after
it. A profile run that goes on past its pattern is a sign of such leftover
bytes.

### MC6809 family

**Pitfalls.** `CWAI` stacks before it waits, so a halt finds no push; the
MC6809 has no VMA (its CNTL0 is XTAL); the 6309's vector base is FFF0; R/`#W`
must be read as E rises on the HD6309.

Two boards run four chips: MC6809 (and HD6309), clocked from EXTAL, and
MC6809E (and HD6309E), whose E and Q the debugger drives. The HD6309 runs
MC6809 code and has its own samples, which run in native mode. **[code]**

`CWAI` stacks the whole context and then waits, so the halt's NMI fetches
its vector without pushing; garbage registers after a halt mean `suspend()`
looked back from the vector read for a push and found none. **[hw]**
`suspend()` injects `RTI` to resume after the `CWAI` and takes a fresh NMI
edge, and its wait for the vector is bounded. Capturing an empty frame
instead would also clear MD on an HD6309 in native mode. `SYNC` needs none
of this: it stacks only after the interrupt ends the wait.

- `Signals::getDirection()` forces the valid bit on for the MC6809: CNTL0 is
  AVMA on the MC6809E but XTAL on the MC6809, which reads low while the
  debugger drives EXTAL. The MC6809 has no VMA; its dummy cycles read FFFF,
  which the cycle sequences match as `N`. AVMA is no use either: it tells of
  the next cycle, not this one. A backtrace or `G` dump that never
  disassembles anything points at the valid bit or the vector base. **[hw]**
- `InstHd6309` uses the vector base FFF0, as on the MC6801 and HD6301, so
  `FIRQ` (FFF6), `SWI2`, `SWI3` and the HD6309 trap match. **[hw]**
- The MC6809E finds fetches by matching cycles, like the MC6809, not from
  LIC, which misses around halts and interrupts. **[hw]**
- `Mems::disassemble()` steps at least one unit: at FFFF an instruction runs
  past memory and NO_MEMORY leaves its length 0, which would make a
  backtrace loop forever and ignore the halt port. A board that floods the
  console and ignores the halt port after a halt suggests a backtrace
  re-disassembling one address; here a fetch mark on a dummy FFFF cycle
  started it. **[code]**

When profiling, the HD6309 drives R/`#W` back high right at E's fall, so
read R/`#W` as E rises; compare only the kind of a dummy cycle such as the
read of `$FFFF`. **[hw]**

`CWAI` and `SYNC` halt and step correctly in both emulation and native
mode.

### MC68HC12

**Pitfalls.** Only the special expanded modes put every bus cycle on the
bus from reset; the CPU12 prefetches into an instruction queue, so a
program read does not mark an instruction start -- IPIPE1:0 do.

Board for the MC68HC912BD32 (80-pin QFP), identity `MC68HC12B`. Pin
positions are the board's assignment, not facts from a manual.

- The Teensy holds BKGD (P44) low and MODB and MODA high through reset,
  which selects special expanded wide mode. That mode comes out of reset
  with R/`#W`, `#LSTRB`, IPIPE1:0 and `#DBE` as bus signals and IVIS set,
  so internal accesses show on the bus. A normal expanded mode enables
  only E and leaves IVIS clear. **[doc]**
- After reset BKGD is the single-wire BDM pin. Held low it stays
  harmless: a BDM bit starts only on a host falling edge, ENBDM resets to
  0 outside special single-chip mode, and TAGHI acts only after a BDM
  `TAGGO`. **[doc]**
- IPIPE1:0 share the MODB/MODA pins (P43/P42), which become Teensy inputs
  after reset. On the M68HC12 the data movement code is valid at E rise,
  the execution start code at E fall: 01 starts an interrupt sequence, 10
  an even instruction, 11 an odd one. **[doc]**
- The Teensy drives DIVBYP (P45): low gives E = EXTAL/4; high is a test
  mode giving EXTAL/2. It has an always-on pull-down. **[doc]**
- The MC68HC912B32, MC68HC12BE32 and MC68HC(9)12BC32 are pin compatible
  except pins 70-76 (BDLC or CAN in place of Byteflight), which the board
  leaves open but pin 70. There pin 70 is PDLC6/PCAN6, an input after
  reset, so DIVBYP has no effect and E is EXTAL/4. VFP (pin 69) is tied to
  VCC, as the Flash parts need; the ROM parts have no connection there.
  **[doc]** The fixed four-edge bus cycle assumes EXTAL/2, so those parts
  need it counted in four edge pairs; not tried.
- AD0-AD15 sit on P1x/P2x, GPIO6 bits 16-31, readable in one access. In
  a narrow mode the 8-bit data is on port A (P2x), not P1x. **[doc]**
- The on-chip Flash is off in the expanded modes (ROMON resets to 0), so it
  does not shadow external memory. **[doc]**
- Port S is on P3x: SCI0 RxD (PS0) on P30, TxD (PS1) on P31, PS2-PS7 on
  P32-P37. The MC68HC11 firmware bit-bangs its SCI through GPIO, so no
  Teensy UART pins are needed. **[code]**
- PT7/PAI is on P52 and PT0/IOC0 on P55, so the Teensy can drive edges
  into input capture or the pulse accumulator for timer samples.

Firmware (`debugger/mc68hc12`):

- DIVBYP is driven high, so each EXTAL period is half an E cycle and one
  edge of each pair moves E; `sync_e()` finds which one while #RESET is
  low, before any bus cycle. E then follows EXTAL about 20ns after the
  edge, so a bus cycle is four fixed edges: quiet (address, R/`#W` and
  `#LSTRB` settle about 24ns later), active (E rises), quiet (`#DBE` falls
  about 24ns later on an external read, write data valid), active (E
  falls). E is read once a cycle only to catch STOP. The CPU12 is static,
  so idle stops the clock. **[hw]**
- The bus is synchronous: no wait input, and `#DBE` only marks an
  external read, a quarter cycle into E high. **[doc]**
- AD is driven only while `#DBE` is low, an external read: internal reads
  shown under IVIS and free cycles leave the bus to the CPU. **[code]**
- Reset leaves EXSTR=11 in MISC, a three-cycle stretch with E held high
  for seven EXTAL pairs and `#DBE` low only after the last quiet edge;
  `configSystem()` clears it and stops the COP, and until then
  `prepareCycle()` walks a read, sampling `#DBE` at every quiet edge, until
  `#DBE` or E falls. A sample has to clear the level as well as set it:
  one that only set it saw the first high and missed every reset read.
  **[hw]**
- Injection is address-keyed, as on the P8095BH: a sequence ends with a
  `BRA` back to the park ($FF80), and the CPU waits in the refetch. A
  vector read (reset, SWI, `#XIRQ`) is answered with the park, where
  `BRA *` waits while the stacking is captured by address. **[code]**
- `restore()` builds the RTI frame with pushes, so a stack in internal RAM
  works; an external one is answered from the frame. A user PC within
  about 30 bytes after the park would end it early. **[code]**
- `#XIRQ` halts and steps: `restore()` clears CCR.X. RTI checks for a
  pending interrupt after its first queue refill, so `#XIRQ` asserted
  before RTI's first PC fetch runs nothing, and asserted at the execution
  start (IPIPE) runs two instructions. Asserted before the E fall of the
  second cycle after the restore, the second refill, it runs exactly one.
  WAI stacks before it waits, so a halt there takes the frame from the
  writes before the vector. **[hw]**
- The only injected SWI is `save()` at reset, when I and X are set, so no
  pending interrupt can take its place. **[code]**
- Every SWI vector read stops the run: the queue can fetch a breakpoint's
  SWI long before it executes, so only the stacked PC tells a breakpoint
  from the program's own SWI. The latter's frame is written back and the
  run goes on in its handler. **[code]**
- No cycle table: the backtrace replays the queue from IPIPE1:0 as
  CPU12RM 8.8 describes (`queue_mc68hc12.cpp`, host test
  `test/mc68hc12/test_queue_mc68hc12`). **[code]** A page 2 instruction
  signals two starts, at its $18 prebyte and again at the opcode after it
  with the queue held (LAT), so the replay folds the second into the
  first. **[hw]**
- Memory reads and writes check `devs->isSelected()` once per word and go
  to the emulated memory directly: the per-byte virtual `Mems::read()`
  chain cost more than the rest of the bus cycle. A device decodes an
  aligned block of four, so a word is in or out whole. **[hw]**
- The samples use EMULS and IDIVS inline; IDIVS's remainder is the
  `B-Q*F` that other targets multiply back. **[code]**

### MC68HC16

**Pitfalls.** The pin functions come from the data bus at reset: hold
DATA2 and DATA7 low, and leave DATA8, DATA9 and DATA11 high, or FC2:0 and
ADDR23:19 come out as chip-selects and the bus control pins as port E.
IPIPE1:0 carry two phases per bus cycle, so one sample is half the story.

Board for the MC68HC16Z1 (144-pin LQFP, case 918); built, not yet brought up.

- While `#RESET` is low the Teensy drives DATA1, DATA2 and DATA7 low.
  DATA2 gives FC2:0, DATA7 alone gives ADDR23:19 whatever DATA6:3 are,
  and DATA1 gives `#BR`/`#BG`/`#BGACK`, which only matters for
  arbitration. DATA8 keeps the bus control pins, DATA9 the `#IRQ` pins and
  MODCLK, and DATA11 must stay high for normal operation; the weak
  internal pull-ups hold any line not driven. **[doc]**
- `#RESET` is open drain both ways: the MCU stretches an external reset by
  512 CLKOUT cycles and asserts it on internal resets. The pull-up is on
  the Teensy side of the level shifter, so drive it low or release it,
  never high. **[doc]**
- MODCLK is tied low, so EXTAL (P47) is the system clock with the PLL off;
  CLKOUT comes back on P52. **[doc]**
- IPIPE1:0 encode phase 1 (START, FETCH) and phase 2 (ADVANCE,
  EXCEPTION) of the pipeline state; latch them on the `#AS` and `#DS`
  edges. Under FREEZE they become DSI (an input) and DSO. **[doc]**
- P40-P43 and P46 read as one GPIO9 group: `#AS`, R/`#W`, IPIPE0, IPIPE1
  and `#DS`, so both IPIPE phases come with the strobes. **[code]**
- ADDR23:0, SIZ1:0 and FC2:0 share P30-P37 through four 74HCS153
  selected by ASEL0 (P54) and ASEL1 (P55): 00 A0-A3/A16-A19,
  01 A4-A7/A20-A23, 10 A12-A15/SIZ, 11 A8-A11/FC. The selects are
  walked once per cycle in Gray order; the bus holds still meanwhile, as
  the CPU waits on the bit-banged EXTAL or, in a run, on `#DSACK1`.
  **[code]**
- `#DSACK0`/`#DSACK1` (P44/P45) end external cycles. `#AVEC` is tied low,
  so every interrupt acknowledge is autovectored. `#IRQ7` (P50) and
  `#IRQ1` (P51) reach the Teensy; `#IRQ6`-`#IRQ2` are tied high. **[doc]**
- TSC is tied low and `#BKPT` tied high, since a low `#BKPT` at reset
  enables background mode. The bidirectional `#HALT`, `#BERR`, `#BR` and
  `#BGACK` have 10k pull-ups instead of ties: the MCU drives `#HALT` low on
  a double bus fault. VSTBY is grounded, so the standby RAM keeps
  nothing across power-off. **[doc]**
- The Z2, Z3 and Z4 share the 144-pin assignment; the Z4 differs only in
  its MCCI serial pins (28-37), which the board leaves open. The Z2 and
  Z3 masked ROM wakes enabled because DATA14 is high at reset, and its
  accesses never reach the bus; a mask-programmed BOOT bit may also take
  the reset vectors from it. The CK and CM parts are 3.3 V. **[doc]**

#### Debugger **[doc][code]**

Written against the manuals, before bring-up; every timing and
landing point below is marked `ATTENTION:` in `debugger/mc68hc16/`.

- The CPU is parked in S1 of a bus cycle with EXTAL stopped: external
  clock operation goes down to dc, so the timers and watchdog hold too.
  Outside a run `#DSACK1` stays asserted, so every external cycle ends at
  its first S2/S3 fall with no wait states.
- A run clocks EXTAL from FlexPWM3 at a constant 10 MHz, the fastest the
  full regress passes at (see Clocking a run from a timer). `#DSACK1` is
  negated then: each cycle waits until it is served, and the on-chip
  timers keep running while the CPU waits. The MC68HC16Z1PV16 needs
  EXTAL high and low for 29.8 ns, so 150 MHz / 10 = 15 MHz is the highest
  rate the PWM can make for it. **[hw]**
  Sequences are address-keyed and end in `BRA` to their origin or a jump
  to a named exit.
- CSBOOT covers 00000H-7FFFFH from reset and ends a cycle by itself after
  13 waits, and the software watchdog is on. `setupBus()` runs right after
  the reset vector, before the program runs: it clears SYPCR (write-once,
  so a program cannot turn the watchdog back on) and CSORBT.
- The SIM holds off an external reset until the bus cycle in progress
  ends, and with the bus monitor off a parked one never does: `R` asserts
  `#DSACK1` for the first clocks of `#RESET`, before the straps go on the
  bus. **[doc]**
- Interrupt acknowledges (FC=7) end on `#AVEC` and autovector, even with
  `#DSACK1` asserted too, as the `#IRQ1` and `#IRQ7` samples showed on the
  bench; the Teensy drives no data for them.
- Every microsecond the Teensy spends serving a cycle stretches it, as
  the CPU waits on `#DSACK1`. Above about 5 MHz the Teensy's time to
  serve a cycle, not the clock, sets a run's speed. A run polls the halt
  switch, whose
  `yield()` services the USB, only every 16 bus cycles; the devices
  ration their own polling. A CPU in `WAI` is polled every 16 clocks, so
  the devices that wake it and the watchdog are serviced often. **[code]**
- The clock delays are minimums: EXTAL high at least 29.8ns, and #AS and
  #DS were seen within 30ns of a falling edge. A read is looked up and
  driven while S2's EXTAL is high; a write is
  stored while S4's is. `delayNanoseconds()` adds its own overhead to
  each delay, and counting the delays from the edges instead, without
  it, corrupted the bus: keep them as they are. **[code]**
- A memory read, byte or word, moves the aligned word, from which a byte
  read takes its own lane. `MemsMc68hc16` is `final`, so its calls bind
  statically, and the device check is one call per cycle. DMAMEM is no
  faster than EXTMEM here: both are cached. **[code]**
- Halt takes `#IRQ7` (level-7 autovector, 002EH). The frame is PC at SP
  and CCR below it, SP down by 4; the stacked PC is the next instruction
  plus 6 for an interrupt, the `SWI` plus 8 for `SWI`. RTI subtracts 6.
- Step can't use `#IRQ7`: it is synchronised over two clocks and taken
  at the boundary after, by when a 2-byte instruction has been followed
  by another. Step serves the instruction's own words from memory and
  every other program read as `SWI`, so whatever runs next, a branch
  target or a handler included, breaks. The CPU16 has no trace mode, and
  its length comes from `InstMc68hc16::length()`, a table checked
  against libasm by `test/mc68hc16/test_inst_mc68hc16`. **[doc]**
- `SWI` leaves the interrupt mask as it was, so a break chains `#IRQ7` onto
  it: the `SWI` vector is answered with NOPs and `#IRQ7`, taken at their
  first boundary, sets IP to 7. Save and restore then always run masked;
  the `SWI` frame holds the program's state. **[doc]**
- Registers are saved with `PSHM D,E,X,Y,Z,K,CCR` and `PSHMAC`, decoded
  by write address, and restored from an image staged just below SP:
  `PULMAC`, `PULM`, `RTI`. A stack in the on-chip standby RAM shows
  nothing on the bus, so it cannot be saved. K is EK:XK:YK:ZK from bit 15
  down (CPU16RM Figure 2-1). **[doc]**
- Instruction boundaries come from IPIPE: phase 1 is read at `#AS`,
  phase 2 late in the cycle, and `PipeMc68hc16` maps each START back to
  its opcode's fetch. In a run, phase 1 is read as `#AS` is first seen
  low and phase 2 at the last read before it negates (User's Manual AC
  timing 100-104). Phase 1 lasts only until S2, about half a clock after
  `#AS`: at 15 MHz a poll misses it, and halts and the disassembly lose
  the instruction starts. A FlexIO1 latch at `#AS` reads phase 1 right,
  but takes the pads from GPIO9, and a run polled through FlexIO is too
  slow. **[hw]**
- The ACIA sits at FFE00H, outside the SIM's register block; samples
  reach it as `,Z` with ZK:IZ set from the reset vector, and keep data
  in bank 0.
- An interrupt stacks only PC and CCR, unlike the HC11 the samples were
  ported from: each sample's handler saves what it uses with `PSHM` and
  restores it with `PULM` before `RTI`. **[doc]**

### SCN2650

**Pitfalls.** A conditional branch not taken skips its indirect address
read, two cycles fewer than its row; `HALT` makes no bus cycle after its
fetch.

The profile is `tools/cycles_scn2650.py`, recorded as
`tools/scn2650-cycles.jsonl.zst`. An indirect `BCTR`, `BDRA` and the like
make two bus cycles fewer than its row when they fall through. **[hw]**
`InstScn2650::notTaken()` tells a fall-through by the address after the
operand, so the run loop and the matcher don't take those cycles from the
next instruction.

### TLCS90

**Pitfalls.** On-chip RAM and I/O (FEC0-FFEF) never show on the bus, so
instructions touching them make fewer cycles than their rows; a word access
at FFFF wraps to 0000.

The profile (`tools/cycles_tlcs90.py`, recorded as
`tools/tlcs90-cycles.jsonl.zst`) matches the manual's bus-cycle tables on
every opcode. **[hw]**

- `JRL` and `CALR` count their 16-bit displacement from the opcode + 2, as
  an 8-bit `JR`'s does.
- A word access at FFFF goes on at 0000; `r` and `w` allow it.

On-chip RAM (FEC0-FFBF) and I/O (FFC0-FFEF) never show on the bus; FFF0-FFFF
is external again. The profile keeps its operands and SP in external memory,
and leaves out patterns that would point SP at on-chip I/O. **[hw]**

### TMS320C15

**Pitfalls.** The bus timing has no slack: the 10 ns delays that replaced the
commented-out debug pin calls are the setup margin; don't remove them.

The profile (`tools/cycles_tms320c15.py`, recorded as
`tools/tms320c15-cycles.jsonl.zst`) matches the tables on every opcode.
**[hw]** The debugger drives `#BIO`, so `BIOZ` is recorded both taken and
not.

The fetch data settles as late as the CLKIN edge after it. **[hw]** 10 ns
after each drive and before each sample settles it 40 ns before that edge
and 110 ns before `#MEN` rises, and holds it 120 ns after, with analyser
leads on the bus too. Too little margin shows as a few wrong mandelbrot
pixels at the same places, not every frame.

### TMS7000 family

**Pitfalls.** Register-file accesses, pushes and pops included, never reach
the bus; `TRAP n` makes two bus cycles.

One table serves the whole family, and the profile
(`tools/cycles_tms7000.py`, recorded on a TMS70C02 as
`tools/tms7000-cycles.jsonl.zst`) takes any family member. **[hw]**
`TRAP n` makes two bus cycles, its vector reads.

An interrupt pending at a halt is taken at the save's first fetch and would
feed the injected bytes to its vector reads. `park()` runs `JMP $` there
until it has run twice without one, or an entry shows and the save starts at
the handler. The entry is an acknowledge cycle at address 0 and two vector
reads, but a TMS70C02 can be at the vector reads already at the halt, so a
read at FFD0 or above counts too. **[hw]**

### TMS370

**Pitfalls.** The oscillator fault logic watches CLKIN and resets the CPU
whenever the clock runs slower than somewhere between 20 and 500 kHz; on the
A and B parts no bit disables it. CLKIN therefore runs free from the PWM
through every sequence and at the prompt, and `#WAIT` holds each external
access until it is served. **[hw]**

- `#WAIT` is sampled at a SYSCLK rise (setup 0.25tc+70, hold 0), and with
  the auto wait state not before the third SYSCLK of an access. It is
  negated just after a rise, a whole tc ahead of the rise that samples it,
  and asserted again once `#EDS` rises. **[doc]**
- Write data is valid only by the SYSCLK fall after `#EDS` falls, not at
  the fall. **[doc]**
- OSC POWER (SCCR0.6) must stay clear above 3 MHz SYSCLK. **[doc]**
- A flash stops the clock and sets OSC FLT FLAG (SCCR0.4), which no reset
  clears. **[hw]**

An interrupt pending at a fetch aborts it and reads its vector next.
`takePendingInterrupt()` runs `JMP $` at the halt until it has run twice
without one, so an interrupt recognised a boundary late is taken there too;
one run is not enough. Injected bytes showing up in the saved state reveal
a missed interrupt; only many halts in a row expose it. **[hw]**

The wait for `#EDS` gives up after 1024 CLKIN cycles at the slowest rate,
2 MHz, four times the longest instruction (DIV, 63 SYSCLK cycles), and prints `?halt: no bus
cycle`, so a CPU that stops issuing bus cycles doesn't wedge the board.
**[code]**

### Z8

**Pitfalls.** The stack is external, and the tables must see
`EXTERNAL_STACK`; on the Super8, register sets C0-FF are banked, and a
working-register write can reload the register cache from the wrong bank.

One board each runs the Z86C91 and the Z88C00 (Super8), sharing
`PinsZ8`. The profile (`tools/cycles_z86.py` and `tools/cycles_z88.py`,
recorded as `tools/z86-cycles.jsonl.zst` and `tools/z88-cycles.jsonl.zst`)
matches the tables. **[hw]**

- `EXTERNAL_STACK` is defined in `inst_z8.h`, not `pins_z8.h`, which the
  instruction tables don't include; without it their `#if` drops the pushes
  and pops of `PUSH`, `POP`, `CALL`, `RET` and `IRET`.
- `NEXT`, `EXIT` and `CALL @RR` make 3, 3 and 1 external cycles, as
  `z88.txt` says; keep `inst_z88.cpp` in step with it.

`HALT` on the Z86C91 and `WFI` on the Z88C00 read the byte after them,
then the bus stops. **[hw]**

`restore()` enables interrupts with `IRET`, which pops FLAGS and PC and sets
the global enable (IMR bit 7, the Super8's SYM bit 0): it loads the mask
without that bit, moves SP down three bytes and feeds `IRET` the pops.
`IRET` reads the byte after it before popping, on both chips. **[hw]** With
the enable clear it still ends in `JP`, and so it does on the Super8 while
FIS is set, since `IRET` then returns from a fast interrupt without popping.
**[doc]**

Normal builds don't follow the Super8's fast interrupt. It pushes nothing,
and the run loop, which finds instruction boundaries by counting each
instruction's bus cycles, loses them at the first one. `Z88_FAST_INTERRUPT`
in `pins_z8.h` follows IP, SYM's fast enable and FIS from the restored state
and the program's immediate writes to them (an IP written from a register,
or by `NEXT`, `ENTER` or `EXIT`, isn't followed). It takes a fetch at IP
after an opcode fetch and a read of the byte after it as an entry, and
counts a fast `IRET` as two bus cycles. Checking every bus cycle for an
entry slows the bus of both chips, so it stays out of normal builds. A
mandelbrot built with fast interrupts tests it: without the option it stops
almost at once. **[hw]**

On the Z88C00, only setting RP, RP0 or RP1 refreshes r0-r15 in the register
cache. **[hw]** `write_reg()` and `restore_sfrs()` write RP0 and RP1 while
RP0 points at C0, so refreshing on any such write would load the C0 bank,
and `restore()` would write it over the user's values. `r` shows the
cache, so check a register set with `=` after a `G` or step, with RP0 other
than C0.

Super8 indirect and stack accesses to C0-FF reach register set 2, not the
registers a direct address names, so a working register at C0 can't point
at itself. The profile keeps RP0 at 20. **[hw]**

### PDP-8 (IM6100 / HD6120)

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

### CP1600

Draft board for the CP1610 (40-pin DIP); not built yet.

- The CP1610 runs from +11 V VDD and −2.2 V VBB, both ±5 %, not the
  CP1600's +12 V and −3 V. An NJW4132 boost makes VDD from VCC; an LM2776
  inverter and a TPS72301 regulator make VBB, and a Schottky diode keeps
  VBB from rising above GND while the rails come up. **[doc]**
- A UCC27324 powered from VDD turns the Teensy's PHI1/PHI2 (P46/P47) into
  the two-phase clock, which must reach 10 V, rise and fall within 15 ns
  and never overlap; see
  [Driving a high-voltage clock](#driving-a-high-voltage-clock). **[doc]**
- `#INTR` is open drain with a 10 kΩ pull-up. The Teensy (P50) pulls it
  low to halt, and a 74LVC1G02 and 74LVC1G06 pull it low while `#PCIT`
  and `#ENSWI` (P34) are both low, so a `SIN` traps only when the Teensy
  enables it.

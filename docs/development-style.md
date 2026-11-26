# Development style

Notes on bringing up targets in this codebase: what is not obvious from the
source.

Each claim is marked **[hw]** when it was verified on the bench, **[doc]** when it
comes from a datasheet or manual, and **[code]** when it is a property of this
codebase.

The General part holds what applies to every target. Each file under
`docs/arch/` opens with that chip's pitfalls; read them before touching
its code.

---

## General

### Injection models: sequential cursor vs address-keyed

Two models answer a CPU's reads from an injected byte stream, and they are not
interchangeable:

- **Sequential cursor** — every read consumes the next byte(s) of the stream.
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
instructions, and the CPU's next read is then answered from memory with an
opcode that is not in the sequence. **[hw]** Terminate on *the CPU having read
past the window* instead.

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

A CPU parked mid-transaction resumes at the address it was parked at, which
cannot always be re-read from the pins (a multiplexed bus carries data by
then) and cannot live in the ring: `Cycles::reset()` and `Cycles::discard()`
both clear the head slot, and a dump or an exit is exactly when they run.
Follow the i8080/i8085 shape: `resumeCycle(addr)` takes the address, the
caller keeps it, and `_regs->nextIp()` is where the CPU is parked between
operations.

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
cycles is not caught. The profile image's `X` command hangs on purpose, to
test it. **[code]**

### The halt port **[hw]**

The Teensy builds with `USB_DUAL_SERIAL`: `/dev/ttyACM0` is the debugger
console, and **any byte written to `/dev/ttyACM1` aborts a running CPU**
(`serialEventUSB1()` → `Pins::isrHaltSwitch()` → `_halted`, polled by
`loop()`). It is the only way to stop a run that does not end on its own,
provided the wait loops are bounded.

**It is serviced only from `yield()`.** **[code]** A run otherwise reaches
`yield()` only through console I/O, so `Pins::haltSwitch()` calls it itself;
without that, a program that never enables the console device cannot be
halted. The `yield()` stretches the cycle it runs in, so a bus loop takes it
only when `_devs->loop()` returns true, once in 10 µs on the cycle that
services the devices; every other cycle pays only the time check, plus the
call of a device that counts bus cycles, such as a bit-banged serial clock.
A loop that skips it pads the clock phase it used to fill. **[code]** A halt
that never returns the CLI looks exactly like a wedged board, so try a
program that uses the console before suspecting the board. **[hw]**

**The handler must drain the byte.** The core re-calls `serialEventUSB1()`
from every `yield()` while the port holds data, so an undrained byte aborts
every following run too: every `G` returns at once with a register dump.
**[hw]**

**An abort leaves the emulated USART repeating** its last received
character, which floods `ttyACM0` and buries the CLI prompt. A harness drains
the console after any abort, and stops an interactive sample the way the
sample expects (NUL for `samples/z280`) rather than aborting it. **[hw]**

Recovery for an unresponsive board, in increasing violence: Ctrl-C (`0x03`),
Ctrl-Space (`0x00`), then the halt port. If none draws a reply the firmware is
stuck in a wait: the watchdog reboots it if the wait feeds nothing (see
"Bound every wait loop"); otherwise only a reflash brings it back.

### Lock a bench before using it **[hw]**

Several benches can be on USB at once, each a Teensy named `bench1`,
`bench2`, ... in the order first seen (`scripts/bionic-bench.py rename` gives
a clearer name), one bench per session. A session takes one with
`scripts/bionic-bench.py lock NAME`, or queues with `reserve`;
`bionic-control.py`, and every tool built on it, then talks to the bench the
session holds and refuses one another session holds. The bench's two USB
serials, console and halt port, follow from its Teensy's serial number.

A logic analyzer is locked the same way: a resource added with
`scripts/bionic-bench.py add NAME kind=logic mcp=URL`, its Logic 2 MCP
server. `logic-analyzer.py` captures on the one the session holds (or
`BIONIC_LOGIC`, or the only one) and refuses one another session holds;
Logic 2's MCP tools called directly cannot check, so lock the analyzer first.
A capture it starts keeps at most 10 ms and stays open for a look until the
next start closes it; one that never triggers is stopped, not exported, and
an export takes only that capture's channels.

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
- The 8086 family fetches at the handler before it pushes the return IP: the
  bus shows the vector reads, the FLAGS and CS pushes, a fetch at the handler,
  then the IP push. Start the frame at the vector read, answer handler fetches
  with NOPs until all six bytes are pushed, and park at the next fetch.
  **[hw]**
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
`bionic-control.py flash`) adds `y n`: `yield()` on one in `n` halt polls.
`y 1024` makes the pending interrupt common; a power cycle puts it back to
`y 1`. The test is the regress plus five runs of `haltgo=30` at `y 1024`.

Every family in "Halt through an entry the CPU takes itself" passes it, by
the entry or the masked save named there, with the remedies in its own section
where one was needed. The exception is the TMS9980 family: on its PWM clock
the TMS9980 and TMS9981 have passed only the plain regress.

### Compare a pin read against `LOW`, never `HIGH` **[code]**

`digitalReadFast()` may return the masked register bit, `1 << bit`, rather
than 1: truthy but **not equal to `HIGH`**, so `read() == HIGH` fails on pins
whose bit position is not 0. `LOW` is 0, so `== LOW` and `!= LOW` are exact.
Write the test that way even where the current expansion is safe.

### Change a coded input through a neutral code **[code]**

`busWrite()` clears bits before it sets them, so for one store the
port shows the old and new values ANDed. Data the CPU latches later
doesn't mind, but a code the CPU samples every clock can: TMS9980 INT2-0
codes 4 and 3 AND to 0, which resets. Go through the code that means
none (7 on the TMS9980), or negate the strobe that qualifies the code
(#INTREQ on the TMS99105) while it changes.

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
  itself: every overshoot is lost on each bus cycle.
- Write the measured phase next to the constant (`= 5;  // 30 ns`), and
  re-measure when the code around it changes.
- Work done inside the phase (a pin read, a port read, a bus drive) counts
  toward it, often enough to need no delay at all.

### A constant delay is not the same delay at run time **[hw][code]**

With a constant argument the compiler works the cycle count out itself:
`delayNanoseconds(0)` vanishes, and a small constant makes a shorter phase
than the same value read from a table at run time, whose arithmetic alone
holds a phase near 20 ns. Moving tuned values from a run-time table into
`constexpr` constants therefore moves each phase by its own amount. On the
P8051 the same table shifted XTAL phases by −2 to +12 ns, site by site, with
no relation to the value.

- Re-measure every entry on the bus after such a change. A calibration loop
  that pulses a spare pin runs different code from the bus loop and predicts
  neither.
- An entry under about 20 sits on a floor where a raise does little, so a
  phase found short wants 20 or more, not a few nanoseconds more.
- Check the shortest phase and period over the whole capture, not at chosen
  positions in a cycle: a short phase on a rarely taken path (the second
  pass of a wait loop, a write cycle) is missed otherwise.

### Squeezing a Teensy-clocked bus cycle **[hw][code]**

Where a bus cycle's time goes is found by measuring, not by reading the code.
The S80296SA's went this way:

1. Capture XTAL1 with the bus strobes and time each phase from one bus state
   to the next. The phases the software stretches stand out against the
   ones the delays make.
2. Stamp the suspects with `ARM_DWT_CYCCNT` into a scratch buffer, print it
   after the run, and drop the stamps afterwards. A stamp costs about 50 ns,
   so subtract it.
3. A `yield()` on every cycle, from the halt switch's poll, cost about
   160 ns (see "The halt port" for where the poll runs).
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
   is staged in the port with `busWrite()` while the bus is still an
   input; c61 only turns the port to output.
8. Give every branch of a phase its own delay. A debugger-injected read
   skips the memory lookup, and with no delay in its branch the CDP1802's
   c40 low half shrank to about 30 ns: the CPU kept fetching the byte
   staged before, while the firmware went on as if each fetch were served.

### Clocking a run from a timer **[hw][code]**

A CPU whose clock may stop can run from a free-running FlexPWM output
instead of bit-banged edges, which is the way to a CPU whose clock goes
through a PLL. The MC68HC16 does this on EXTAL (P47); reset, step and
injected sequences stay bit-banged. CPUs whose clock must not stop run
every sequence from it: the TMS9980 and TMS9981 on CKIN (pin 5), the
TMS370Cx5x on CLKIN (pin 5), the INS8060 and INS8070 on XIN (pin 5), the
8085 on X1 (pin 6), and the P8086 and D8087 boards' x86 and 8087 on CLK
(pin 29). The static SCN2650 runs from it on CLOCK (pin 5) too: the timer
paces the cycles, where bit-banged edges needed a tuned delay for each.

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
  stopped clock allowed, catches the address changing. The 8086's write
  data follows T2's falling edge: wait for it by the counter, not a fixed
  delay from T1.
- A CPU with dynamic logic loses its state, at random, while its clock is
  stopped for seconds: the V20/V30 (tCYK 500 ns at most) kept running
  from a back-to-back reset and run, and lost IP across a park at the
  prompt. Such a CPU always runs from the timer.
- After raising the wait input, wait for the CPU's own sign that the wait
  is over (the 8086's status going passive) before counting edges to the
  data hold: the V20/V30 take READY an edge earlier than the 8086 does.
- Never poll the FlexPWM counter in a wait: a peripheral read, it comes
  back late by up to a period now and then. It and the CPU's cycle
  counter count from one PLL, so read the PWM counter once when the rate
  is set (the quickest of many reads: its latency varies) and take every
  phase from the cycle counter after.
- The phase that calibration gives is off by a constant the parts take
  differently: the 8086s need READY and the T1 address read a few counts
  later than the V20/V30 do. Reset at a slow, safe rate, tell the part,
  then switch to its own rate and phase from a table; `K` overrides both
  for a sweep.
- Read the next cycle's address in the masked stretch that ends the
  current one: an interrupt taken after the release, before the address
  is in, makes the read miss it (before T1 on the 8086; the HD641016
  drops its address at T2 for the write data, and a USB interrupt on
  unmasking took a push's data for its address). A held cycle holds only
  after the address phase, so where the address shares pins with write
  data nothing may run between serving one cycle and reading the next
  address: service the devices while the cycle is still held, and keep
  interrupts masked from the release until the next address is in. Keep
  the new cycle out of the ring until the caller asks for it: while the
  ring is held, the head slot is still the cycle the caller is finishing.
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
  state wherever it misses an edge; a `yield()` before every fetch made
  the 8085 slower at 10 MHz X1 than at 9.375 MHz. Measure each phase of
  a cycle in a capture before blaming the rate.
- Holding every cycle costs a wait state each. In a run, a plain memory
  read can go without one: decide from the status as soon as it is valid,
  leave the wait input released, stage the data in the port while it is
  still an input (`busWrite()`, no read of the data register), and
  drive it the moment the read strobe falls. Every 10 us hold a fetch to
  run the devices, which may change an interrupt input only while the
  CPU waits, and to poll the halt switch. It only pays while the Teensy
  keeps up: on the 8085 at 10 MHz X1, an ALE came 90 ns after a held
  write's strobe and was over before the Teensy looked, and zero-wait
  reads worked only at 6 MHz, slower than every cycle held at 10 MHz.
- Measure the bit-banged machine cycle against the CPU's rated clock
  before moving it to a timer. The CDP1802's bit-banged clock already
  ran near its 5 MHz, a 1.73 us machine cycle against 1.60 us, so a timer
  could gain at most that gap, and holding each cycle with `#WAIT` cost
  more than that. Squeeze the bit-banged cycle instead. The CDP1802
  pauses on any falling clock edge with `#WAIT` low, ignoring that edge;
  the CDP1804A family pauses only as TPA or TPB rises.
- The Z8400A at 4.05 MHz samples `#WAIT` about 160 ns after `#MREQ`, and
  the Teensy needed about 300 ns from the strobe to the release, so every
  cycle took one more state and the Z80 stays bit-banged.
- A strobe can trail its request by part of a clock: the HD64180S drops
  `#WR` 50-60 ns after `#ME`. Wait about a clock for it before deciding a
  cycle has none; deciding at once released a write unheld, and it was
  lost.
- Keep bookkeeping off the release: poll the strobe's end straight from
  the port, and advance the cycle ring once the wait input holds again.
  With the ring advanced first, a Z8018010 at 16.67 MHz EXTAL let an NMI's
  push run unheld, and every Z180-family chip ran slower. Nothing slow
  runs between the release and the next hold either: a lookup that may
  miss the cache goes before the release.
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

### Sampling a CPU output every clock by DMA **[hw][code]**

The 8086's queue status (QS1, QS0) says which fetched byte the CPU takes
and when its queue empties; `debugger/i8086/qs_i8086.cpp` keeps it in a
ring, one DMA sample per CLK, so the bus loop never reads it.

- DMA can't read the fast ports (GPIO6-9). Move the pad to its slow port
  (clear its bit in `IOMUXC_GPR_GPR26`-`29`); nothing else may read it
  there.
- A slow-port read takes about 100 ns. Two channels, one per port,
  could not both keep up at 5 MHz: one kept 7 samples in 8. One request
  reading both ports (`SOFF` to the second, `SLAST` back, `CITER` 1)
  keeps every clock and pairs the bits in one slot.
- Time the request from the clock: FlexPWM3's trigger at CLK's rising
  edge restarts a FlexPWM2 delay timer through XBAR1 `EXT_SYNC`, and that
  timer's compare asks for the sample. It never counts to its reload, so
  set `LDMOD`, or its compare values never load.
- QS changes about 100 ns after CLK rises: sampled at the edge, the two
  bits came from different clocks. 12 bus clocks later is inside the
  window on all four P8086 parts.
- Stop the ring at a known PWM phase, so the latest sample's time is
  exact; matching samples to bus cycles by a time a clock off misplaced
  bytes on the V30.

Decoding the samples (`debugger/i8086/qs_decode_i8086.h`, host-tested in
`test/i8086/test_qs_decode`):

- Count the bytes taken against fetch streams, and with QS1 switch to a
  stream at the flush (QS 10) that began it, not by timestamps: the V30
  takes bytes soon after fetching them. A branch taken to the very address
  the next prefetch would ask for shows only by its flush.
- The stop's own flush begins no stream: ignore the bytes taken after it.
  Leave out the instruction at CS:IP only when it started last: a stop at
  the top of a loop has the same address earlier in the ring.
- On an 8-bit bus the ring may hold one straight stream, begun before it.
  CS:IP places it, checked by instruction lengths against the QS 01
  marks.
- The backtrace line limit applies to the decoded listing too.
- `K 91` dumps the ring raw; replay it against the decoder on the host
  before flashing a guess.

### Driving a high-voltage clock **[hw]**

The TMS9900 (four phases, 12 V), P8080 (two phases, 12 V) and CP1610 (two
phases, 11 V) take clocks the Teensy's 5 V outputs can't reach, and the
phases must never overlap.

- The TMS9900 rev 1 and P8080 rev 2 boards level-shift each phase with a
  common-gate 2N7002 (gate at VCC, source driven by 5 V logic) and a 750 Ω
  pull-up to the 12 V rail: the pull-up alone makes the rising edge, and
  PHI1 stays below the 8080A's 9 V minimum.
- A dual low-side gate driver powered from the CPU's VDD drives both edges
  instead. The CP1610 board uses a UCC27324, with 33 Ω in series with each
  output to damp ringing into the 20-30 pF clock inputs. TMS9900 rev 2
  uses two UCC27324s (PHI1/PHI2 and PHI3/PHI4) with 33 Ω as well, still
  fed from the bit-banged 74ACT164 taps; its phases are a whole PHICLK
  period apart, so the slower turn-off costs no gap. The UCC27444 and
  UCC27624 fit the same footprint: their pins 1 and 8 are enables pulled
  up inside, so a board leaving them unconnected takes either.
- The UCC27624 is the best of the three: its delay is the same for both
  edges (17 ns typical, 27 ns max) and matched between channels within
  2 ns, so the phases keep their widths and gaps; the UCC27444 delays a
  falling input 12 ns more than a rising one, and the UCC27324 10 ns
  (25/35 ns typical), narrowing every gap between phases by as much. The
  P8080 rev 3 board fits a UCC27324 from stock, with pins 1 and 8 tied to
  VDD so a UCC27624 drops in. For four phases use two, and pair in one
  chip the phases whose gap is tightest.

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

### Open-drain reset lines are driven low or released, never high **[doc]**

A reset line the CPU drives too (the MC68000's `#RESET` and `#HALT`, the
MC68HC16's `#RESET`) is open drain both ways. Its pull-up sits on the Teensy
side of the level shifter and the boards carry none, so the Teensy drives
such a line low or releases it to `INPUT_PULLUP`, never high: driven high it
would fight the chip's own assertion and hide it. Each chip's notes say what
the chip asserts the line for.

### Walk the address mux selects in Gray order **[code]**

Boards that multiplex the address, size and function-code lines through
74HCS153s onto one GPIO port (the MC68000 and MC68HC16 boards among them)
read them one select code at a time, once per bus cycle. The selects are
walked in Gray order, so each step changes one select line and the mux
settles once per step, one port read each. The bus holds still meanwhile:
the CPU waits on its wait input, or on a stopped clock, until the walk is
done. A multiplexed address still needs its settle time on a held cycle
(see Clocking a run from a timer).

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
  across several `M` commands and dump it back before concluding anything.
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
every cycle without consulting the tables, plus the matcher's verdict where
one segments cycles into instructions, so one run checks the table, the
matcher and the chip together.

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
interval. A run that ends on its own has exited early: a failure, not a
timing. **[hw]**

### Word-align a sample's variables on a 16-bit bus **[hw]**

A word at an odd address takes two bus cycles on a 16-bit bus. Shortening
code above a workspace can move every variable to an odd address: the V30
then ran `samples/i80186/mandelbrot.inc` slower than the 8086 code, while
the V20 on its 8-bit bus showed no difference. Code moved the same way
pays too: the first fetch after a jump to an odd address is a single byte,
so interrupt handlers and the routines called per character start at even
addresses as well. `align` leaves its gap unwritten, so nothing may fall
through into an aligned label. Put `align 2` before word variables, and
compare the two variants' frame times on a 16-bit bus part before trusting
either.

### Samples on another console device **[code]**

`io = "SCI"` in a sample's table routes the console through that device with
the `I` command for that sample only. Reset does not change the selection, so
the script reads the enabled device from `I`'s listing first and switches back
to it afterwards, pass or fail.

### Another architecture's image, unchanged **[hw]**

The V20/V30 run `samples/i8080/mandelbrot.hex` as is, in 8080 emulation
mode (`samples/v30/mandelbrot_8080emu.asm`):

- Load it outside segment 0: there its RST 5/6 interrupt types (EFH, F7H)
  would have their native vectors inside its own code. The Makefile puts
  an extended linear address record (type 04) before the unchanged hex.
- Every interrupt, and the halt's NMI, leaves emulation for a native
  handler; `IRET` resumes it. The 8086 sample's handlers serve the 8080
  program's queues through DS0, the same layout at the same addresses.
- `P` sets the 8080EMU area, the 8080 code's window from its code
  segment's base: `D` and the backtrace decode it as 8080 code, and a
  break point there is an 8080 HLT (76H), since INT 3 (CCH) is CZ. The
  halt is stopped through the NMI the bus loop already raises, and the
  save backs IP up past it; nothing is added per bus cycle.
- The trap flag steps 8080 code, and the mode changes too: BRKEM into
  emulation, IRET back to it, on the V20 as an IRET on the 8088 steps.
  Stepping by break points was tried and dropped.
- The 8086 manuals hold interrupts, and so the trap, off for one
  instruction after a segment register load: a step over MOV DS,AX would
  also run the next one. The harness loads DS early, so the instruction
  before its BRKEM steps alone; the V20's behaviour there is not tried.
- CP/M runs the same way (`samples/v30/cpm80.asm`) at 1000:0000; its
  BIOS polls, so no native handlers. The harness sets DMAX to 1 so the
  BIOS's 16-bit DMA addresses land in that segment.

### Unix V6 installs itself under simh **[code]**

`debugger/pdp11/tools/v6-install.py` types V6's setup at the console of
open-simh's PDP-11, an 11/40 with the tape's disks `v6-fetch.py` cut on
an RK11, checks each step's output, and puts the disks in the board's
image slots. It first ran on the board, which took seven minutes; under
the simulator it takes seconds. What V6's console taught it:

- The switch register must hold 173030, the board's default, for V6 to
  boot single-user; the simulator's starts at 0, which goes multi-user.
- V6's `kl` driver sends every character with a parity bit, with DEL as
  the fill after a newline: the simulator's console strips bit 7 and the
  script's prompt match skips DEL. A scenario on the board masks the line
  to 7 bits, `I DL11 177`, as the terminal of the time did; the setting
  is a mask, not a count, since 8 is no octal digit. Until `stty -lcase`
  the shell echoes in upper case; `#` erases and `@` kills, so a file
  typed in with `cat` may hold neither. `mkconf` reads device names up
  to `done`.
- On the board V6 drops keys typed faster than 50 ms apart, so a
  scenario types at that pace; the simulator feeds its console a key at
  a time and takes them at full speed.
- On the board at 10 MHz from the PWM, `cc mkconf.c` failed about half
  the runs with compiler errors that moved from boot to boot (`No code
  table for op 0`, `s redeclared`), while the raw disk compared equal to
  itself and `icheck` was clean: the CPU's bus catch, not the disk,
  which the bit-banged clock fixed.

### Reset keeps the board's wiring **[code]**

A device's base address and interrupt vectors are the board's wiring, not
device state: a `Devs` constructor sets the defaults, then only `I dev addr
[rx [tx]]` or a program writing the vector registers changes them. `reset()`,
including the 8251's internal-reset command, clears only the device's own
registers and pending interrupts. Since R keeps whatever a run left
behind, the regress applies `[devices]` before each program it loads, and
`record-scenario.py` puts back any device a scenario moved.

A vector write moves a pending request to the new vector, as `I` does:
left on the old one, a vector of 0 can never negate it.

What a command lists must read back through it as printed, since the tools
save the listing and restore it. libcli caps a number's digits at its
limit's, so a field that echoes a listing takes the widest limit (the
i8096's base prints six digits where `maxData()` has four).

### Reset after uploading, not only before **[hw]**

`R` before an upload stops the CPU but resets from the vector already in
memory, the *previous* program's; uploading does not touch the PC. It shows
when samples start at different addresses (on the MC6800, `mc68xx` starts at
9000, the others at 0100): `G` runs the stale program. `bionic-regress.py`'s
`load()` resets again after the upload.

For a CPU that fetches its PC from a reset vector, `[reset]` names the
vector's bytes, most significant first -- `PC = "[FFFE FFFF]"`, or
`"[FFFD FFFC]"` on the 6502 -- and the reset case checks the PC against the
value the loaded sample puts there.

### A scenario opens with the board's banner **[hw]**

Every `.scenario` starts with `key ?`, so the cast opens on the banner,
`* BionicDCT11 * 1.0`: nothing else in a cast names the board, and every
cast's title is just its scenario's file name. `?` prints the state
before any reset, so on a freshly powered bench, where memory is still
blank, load the sample once before recording; otherwise the cast opens
on an undecodable PC.

### A damaged record is rejected, not loaded **[hw]**

The loader takes a record's length from its count field, so a line cut short
would load whatever an earlier, longer line left in the buffer. Each
record's length and checksum are checked; a bad one, or a line that lost
its `S` or `:`, prints `?bad record` and loads nothing. A record lost whole
shows only in the byte count, so `upload_file()` also checks the reported
`bytes uploaded` against the file, and uploads again on either.

### YMODEM over the console lives with lrzsz's habits **[hw]**

`Y` moves files with YMODEM (`debugger/ymodem.cpp`, host-tested against
lrzsz's `sb` and `rb` in `test/test_ymodem`), CRC mode and 1K blocks, as
minicom drives it. lrzsz's receiver flushes its input right after every `C`
and every ACK it sends, so the sender lets the line settle before answering
one: `SETTLE_MS` after a `C`, `GAP_MS` after an ACK, `RETRY_MS` after a
NAK, since lrzsz then takes a second of quiet to clear the line (the host
tests on a pseudo terminal hit the flush every time, USB now and then).
The gap must not follow the header's ACK,
whose `C` comes at once and would be eaten. It doubts the first EOT and now
and then the second, so EOT repeats until acknowledged. It flushes its
output as it exits, which can take the ACK of the closing empty header with
it, so a silence there counts as success (the file was acknowledged
before), and the board waits half a second of quiet (`ConsoleLink::drain()`)
before printing its report, which the flush would eat too; nothing lrzsz
sends after the session reaches the prompt as a command. It stamps a file
without a modification time with garbage, so the header carries a `0`. The
block number wraps past 255: a file over 256 KB is where a `seq == 0`
shortcut breaks. The script waits for the whole ready line before starting
lrzsz, since its tail would reach `rb` as line noise and cost a retry
round. The receiver lets the line go quiet before its one NAK; otherwise a
block that lost a byte would be answered byte by byte, each a NAK and a
retry spent.

### Some CPUs report the PC one byte early **[hw]**

The SC/MP (INS8060) and INS8070 increment PC *before* each fetch, so a stop at
`A` prints `PC=A-1`. Breakpoints and go-until targets still take `A`; only the
printed PC is offset. `pc_offset = -1` in a regress file's
`[breakpoint]`/`[gountil]` tells the script so. To start execution at `A` by
hand, set `PC=A-1`.

### Checking the analyser leads and board files **[hw]**

A preset's labels are only as good as the leads: decode a reset capture
before trusting a new channel set. The register save after reset injects
known instruction bytes on reads and writes known register values, so
swapped strobes (crossed read and write) show at once.

The `schematics/<board>/*_bionic.toml` pin maps are hand-written and have been
wrong (reversed data pins, wrong port, duplicated pin); check them against the
board's netlist (`kicad-cli sch export netlist`) and the PCB's pad nets, not
the other way round.

Keep captures triggered and short. `trimDataSeconds` is the time kept
before the trigger, not the whole window, and it only trims once the
trigger fires, so a stopped, untriggered capture holds the whole run, and
exporting one can fill the disk; closing the capture does not stop the export.
Check that an export starts at minus the pre-trigger time.

A channels table's `display` lists its signals in the order a saved
capture shows them: clocks, then bus status (address latch, read,
write), firmware control such as the debug pin, bus control (ready,
wait), interrupts and resets, and last the address and data buses.

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

A CPU symbol shows every supply pin on its own, visible, with its number:
stacking pins of one name behind a single visible pin hides which VCC and
VSS pins a board actually ties.

### Capturing reset, step and echoitr waves **[hw]**

Every board keeps three captures per chip in
`BusCycles/<debugger dir>/`: `<chip>-reset.sal`, `<chip>-step.sal` and
`<chip>-echoitr.sal`, with `_<coprocessor>` after the chip on a board
that carries one (`80c86_8087-reset.sal`). Their channel tables live in
the target's `samples/<arch>/bench/channels.toml`, one table per board,
chip and kind (`[p8086-v30-step]`), each with its trigger, window,
`display` order and Simple Parallel decoders. To capture a board:

1. Plan the leads. List the signals each kind needs: reset triggers on
   the reset's negation, step on the edge that releases a parked CPU
   (READY, a wait or hold line, or the first strobe of a restore),
   echoitr on the interrupt request. Moving leads is the user's work:
   every request to move them carries the whole table, channel 0 to 15
   with pin and signal, in pin order, not a list of changes. Order the
   chips and kinds so the leads move as few times as possible.
2. Moving a lead means powering the bench off, so every lead change and
   every chip seated is a power cycle: wait for the board to answer,
   read the banner's board and CPU, send one throwaway `R` (the first
   reset after power-up resets twice), and upload the sample and set its
   breakpoints again before arming.
3. Check each channel against its label before saving anything: a clock
   shows its period, RESET is flat outside a reset, a status line toggles
   every cycle. A swapped pair shows as a signal on the wrong channel;
   fix the leads or the table first. Give the lead of a channel that
   never toggles in its captures to something that does, such as status
   bits multiplexed on the top address lines.
4. Open the console before arming and keep that session open: open,
   arm, then send the command. Opening sends `?`, and printing the
   banner can drive the bus (the D8087 board reads the 8087 through
   FSAVE/FRSTOR, about 160 µs that a READY trigger fires on); likewise
   part of a reset capture is the register print, not the reset itself.
5. A wave is for a person to read: keep 50 µs before the trigger, and
   end the window 50 µs after the sequence ends. Probe once at the full
   window (10 ms; echoitr up to 40 ms) to find that end. Each chip gets
   its own tables: an 8-bit bus takes about twice the cycles of a 16-bit
   one.
6. A reset runs through the debugger's register save; the negation edge
   is the one the data sheet times the start from. `R` once the prompt is
   ready runs the same reset as a boot. A pulse-width trigger misses some
   resets: driving the reset pin as an output glitches it first, and at
   power-up the reset is already asserted, so the pulse is not always the
   width the firmware holds. For a boot's own reset, the firmware waits at
   power-up until the console port opens, and the daemon opens it at each
   power-on to see what is seated: hold the board lock
   (`RUN_DIR/board-NAME.lock`, an flock) through the power cycle, arm once
   the board is back on USB, then open the console. A capture armed before
   the power cycle triggers on the power-off edges instead.
7. For steps, run mandelbrot, halt it with the halt port, and send six
   steps as one write (`SSSSSS`): each a restore, the instruction and the
   save. With a coprocessor, step a few of its instructions too. One
   command per step adds the host's round trip between them.
8. For echoitr, run the sample, arm on the interrupt request, and send
   one character: the window holds its receive interrupt and every
   transmit interrupt of the echo.
9. Save each capture with its table's channel names, every digital row
   48 high, and its decoders. The user reviews a board's first reset
   capture (names, order, decoders) before the rest are taken.

Decoders show what the leads allow. A status code decodes on the clock
edge where it is stable. A CPU without address-latch or write strobes on
the connector (an 8086 in maximum mode) still decodes its low
address/data nibble three ways: at the clock edge that ends T1 for the
address, on the read strobe's rising edge for read data, and on READY's
rising edge, the only strobe a write has, for write data.

### Order bench work by image, not by fix **[hw]**

Seating a chip takes moments; flashing the Teensy takes minutes and wipes the
emulated memory. Batch everything one image can test before flashing the next,
and when a fix needs a new image, first finish what the image on the board can
still do.

Name every image by its branch or a feature name (`BIONIC_FIRMWARE`), never
a commit hash: two hashes are hard to tell apart when deciding which bench
runs what, and a flash from a detached checkout without a name is refused.

### Keep commit messages short

A subject line, and a body only when the reason isn't plain from the subject:
one or two sentences on what changed and why, never how the code got there.
Wrap the body at 72 columns.

### Record what a change taught in the same commit

Facts found while making a change — a bus timing, a pitfall, a bench lesson —
go into this document, or the chip's file under `docs/arch/`, in that
change's commit, not a later docs commit.

### Build with and without `ENABLE_SERIAL_HANDLER` **[code]**

The option is off by default, so `devs_*` files can declare a `SerialHandler`
under it without including `serial_handler.h` or the handler's own header and
nothing notices. Syntax-check a target with and without
`-DENABLE_SERIAL_HANDLER` when touching its devices.

### One ITCM overlay per target directory **[code]**

Only one target runs per power-up, so the `teensy41` image (`-D ITCM_OVERLAY`,
on by default) makes the code in the namespaces each `debugger/<dir>/`
declares an overlay: all of them are kept in flash and share one ITCM
address, and `Identity::instance()` copies in the one the identity EEPROM
names. RAM1 holds the shared code plus the largest overlay. An image built
without the flag keeps every target in ITCM, but the rules below hold either
way.

- Code that another directory calls belongs in the shared `debugger`
  namespace, as `DevsZ80` and `DevsMc6800` are; the link fails on a reference
  from one overlay into another.
- A target namespace has no global objects with constructors: startup would
  run them before any overlay is loaded. The build's overlay call check fails
  on such a call. Targets create their objects from `instance()`.
- Each `id_*.cpp` passes `TARGET_OVERLAY(<dir>)` to its `Identity`.
- `teensy_size` counts only the shared ITCM code; the largest `.ovl_*`
  section in `arm-none-eabi-size -A` adds to it.

### Disk images go through DiskUnit **[code]**

`disk_image.h`'s `DiskUnit` is the only way to an image file; `DiskDrives`
is the set `O` and `L` mount. Formats come from the target's devices.
Transfers run inside a held bus cycle and flush once per transfer.

### Drain a transfer's stores before the next bus cycle **[hw][code]**

A device that copies into target memory in EXTMEM or DMAMEM leaves the
stores buffered when it returns. The next cycle's pin reads then stall
behind them and can miss their window: after a cached sector copy, the
8088 read A16-A19 a clock late, in T2, and was fed from the wrong
address. End such a copy with `asm volatile("dsb" ::: "memory")`, as
`SectorDisk::transfer()` does; once per transfer, not once per cycle.

### Devices send console output through consoleWrite() **[code]**

A UART device's character goes out with `consoleWrite()`, never
`Console.write()`: `Devs::loop()` flushes the USB serial once the console
has been quiet for a millisecond. Without it the last character of a burst,
such as the `>` of a CP/M prompt, could stay in the USB buffer while the
CPU only polled the status, until the next character pushed it out.

### SD card libraries stay in flash **[code]**

`ENABLE_SDCARD` is on: `F` lists and `L` loads files on the Teensy's SD
card, and an emulated disk keeps its image there. Like libasm, SD, SdFat
and the formatted output they pull in build without LTO and stay in
flash (`scripts/teensy41.ld`, `scripts/teensy41_layout.py`), so they
take no ITCM. After `L` loads a file, `Devs::fileLoaded(path)` lets a
target's devices mount the images that go with it.

## Per architecture

Each chip's notes live in their own file under `docs/arch/`, pitfalls first;
read them before touching its code.

- [Z280](arch/z280.md)
- [Z380](arch/z380.md)
- [Z80 / Z180](arch/z80.md)
- [TMS9900 family](arch/tms9900.md)
- [6502](arch/mos6502.md)
- [F3850](arch/f3850.md)
- [MCS-48](arch/i8048.md)
- [MCS-51](arch/i8051.md)
- [8080](arch/i8080.md)
- [8085](arch/i8085.md)
- [8086](arch/i8086.md)
- [8096](arch/i8096.md)
- [SC/MP (INS8060)](arch/ins8060.md)
- [INS8070](arch/ins8070.md)
- [MC6800 family](arch/mc6800.md)
- [MC6805 family](arch/mc6805.md)
- [MC6809 family](arch/mc6809.md)
- [MC68HC12](arch/mc68hc12.md)
- [MC68HC16](arch/mc68hc16.md)
- [MC68000 family](arch/mc68000.md)
- [H16 (HD641016)](arch/h16.md)
- [SCN2650](arch/scn2650.md)
- [TLCS90](arch/tlcs90.md)
- [TMS320C15](arch/tms320.md)
- [TMS7000 family](arch/tms7000.md)
- [TMS370](arch/tms370.md)
- [Z8](arch/z8.md)
- [PDP-8 (IM6100 / HD6120)](arch/pdp8.md)
- [DCT11](arch/dct11.md)
- [DCJ11](arch/dcj11.md)
- [CP1600](arch/cp1600.md)

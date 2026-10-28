#include "pins_tms370cx5x.h"
#include "debugger.h"
#include "devs_tms370.h"
#include "inst_tms370.h"
#include "mems_tms370cx5x.h"
#include "regs_tms370cx5x.h"
#include "signals_tms370cx5x.h"

namespace debugger {
namespace tms370cx5x {

using tms370::InstTms370;

// clang-format off
/**
 * CAVEAT: The following timing chart is when AUTO_WAIT_DISABLE is set. Unfortunately any silicon
 * chip in my hand can't respond to interrupts in this configuration (see RegsTms370::reset()).  The
 * current implementation use AUTO WAIT enabled, so that external memory access use 2 SYSCLK
 * cycles. Though TMS370 Family User's Guide says that external memory access use 3 SYSCLK cycles
 * when AUTO WAIT enabled (Table 4-2. Wait-State Control Bits).
 *
 *            __    __    __    __    __    __    __    __    __    __    __    __    __    __    __    __
 *  CLKIN |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *        ____  \    _\___       _\___  \    _\___       _\___  \    _\___       _\___  \    _____       ____
 * SYSCLK     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|
 *        ____________|         | |___________|         | |___________|   |     | |___________          | |__
 *   #EDS             |___________|           |___________|           |___________|           |___________|
 *        _____________________________________________________|                        |____________________
 *   R/#W ______/                                              |________________________|
 *        ______|                       |____________________________________________________________________
 *   #OCF       |_______________________|                                               \____________________
 *        _____   _____________________   ____________________   ______________________   ___________________
 *   ADDR _____>-<_____________________>-<____________________>-<______________________>-<___________________
 *                               __                       __              |_________
 *   DATA ----------------------<__>---------------------<__>-------------<_________>-------------
 *        ______________________v_______________________v_______________________v_______________________v___
 *  #WAIT _____________________/ \_____________________/ \_____________________/ \_____________________/ \__
 */
// clang-format on

namespace {

//   CLKIN: min 2.0 MHz, max 20 MHz
//  tw(CI): min 20 ns        ; CLKIN pulse duration
//  SYSCLK: min 0.5 MHz, max 5.0 MHz
//      tc: min 200 ns, max 2000 ns ; SYSCLK cycle time
// CIH-SCL: max 100 ns       ; CLKIN+ to SYSCLK-
//   SCL-A: max 0.25tc+75 ns ; address valid from SYSCLK-
//    EH-A: min 0.5tc-60 ns  ; address hold after #EDS+
//   tv(A): min 0.5tc-90 ns  ; address valid to #EDS-
//      tw: min tc-80, max tc+40 ; #EDS low width
//  EL-DVR: max tc-95 ns     ; data hold after #EDS+
//  tsu(D): 0.75tc-80 ns     ; data setup for #EDS+
//   EH-DW: 0.75tc+15 ns     ; data hold after #EDS+
//   EH-DR: min 1.25tc-40 ns ; #EDS+ to write data driven
// tsu(WT): min 0.25tc+70 ns ; #WAIT setup to SYSCLK+
//  th(WT): min 0 ns         ; #WAIT hold from SYSCLK+

// CLKIN runs free from the PWM, and #WAIT, low but to end a cycle, holds
// the CPU in each external access until it is served. With the auto wait
// state, #WAIT is not sampled until the third SYSCLK of an access. It is
// negated just after a SYSCLK rise, a whole tc ahead of the rise that
// samples it, and asserted again once #EDS rises.

constexpr uint32_t clkin_hz = 18750000;  // 150 MHz / 8: halves of 27 ns
constexpr uint32_t clkin_min_hz = 2000000;
constexpr auto clkin_half_min_ns = 20;
// Teensy cycles in a CLKIN clock at the slowest rate, so the waits below
// hold at any rate.
constexpr uint32_t clkin_max_cycles = F_CPU / clkin_min_hz;
constexpr auto reset_ns = 10 * (1000000000 / clkin_min_hz);
// Waits for #EDS last 1024 CLKIN clocks; DIV, the longest instruction,
// runs 63 SYSCLK cycles of 4 CLKIN each.
constexpr auto stall_cycles = 1024 * clkin_max_cycles;
// Longer than any SYSCLK: no edge means no clock or no chip.
constexpr auto edge_cycles = 16 * clkin_max_cycles;

inline void assert_reset() {
    digitalWriteFast(PIN_RESET, LOW);
    pinMode(PIN_RESET, OUTPUT_OPENDRAIN);
}

inline void negate_reset() {
    pinMode(PIN_RESET, INPUT_PULLUP);
    digitalWriteFast(PIN_RESET, HIGH);
}

inline void assert_wait() {
    digitalWriteFast(PIN_WAIT, LOW);
}

inline void negate_wait() {
    digitalWriteFast(PIN_WAIT, HIGH);
}

inline bool reset_asserted() {
    return digitalReadFast(PIN_RESET) == LOW;
}

inline bool eds_asserted() {
    return digitalReadFast(PIN_EDS) == LOW;
}

const uint8_t PINS_LOW[] = {
        PIN_CLKIN,
        PIN_WAIT,
};

const uint8_t PINS_HIGH[] = {
        PIN_INT1,
        PIN_INT2,
        PIN_INT3,
};

const uint8_t PINS_INPUT[] = {
        PIN_DATA0,
        PIN_DATA1,
        PIN_DATA2,
        PIN_DATA3,
        PIN_DATA4,
        PIN_DATA5,
        PIN_DATA6,
        PIN_DATA7,
        PIN_ADDR0,
        PIN_ADDR1,
        PIN_ADDR2,
        PIN_ADDR3,
        PIN_ADDR4,
        PIN_ADDR5,
        PIN_ADDR6,
        PIN_ADDR7,
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
        PIN_SYSCLK,
        PIN_EDS,
        PIN_RW,
        PIN_OCF,
};

// Waits up to |limit| Teensy cycles for #EDS to read |low|.
bool wait_eds(bool low, uint32_t limit) {
    const uint32_t start = ARM_DWT_CYCCNT;
    while (eds_asserted() != low) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    return true;
}

// Waits up to |limit| Teensy cycles for a SYSCLK rise, or fall.
bool wait_sysclk(bool rising, uint32_t limit) {
    const uint8_t from = rising ? LOW : HIGH;
    const uint32_t start = ARM_DWT_CYCCNT;
    while (digitalReadFast(PIN_SYSCLK) != from) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    while (digitalReadFast(PIN_SYSCLK) == from) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    return true;
}

// Gives up a wait rather than wedge the board.
void no_bus_cycle() {
    cli.println("?halt: no bus cycle");
}

}  // namespace

PinsTms370Cx5x::PinsTms370Cx5x() : _clock(clkin_half_min_ns, clkin_min_hz) {
    auto regs = new RegsTms370Cx5x(this);
    _regs = regs;
    _devs = new tms370::DevsTms370();
    _mems = new MemsTms370Cx5x(regs, this, _devs);
#ifdef TUNE_CLOCK
    _clockInput = clkin_hz;
#endif
}

#ifdef TUNE_CLOCK
bool PinsTms370Cx5x::setClockInput(uint32_t hz) {
    // The rate reloads at a period start, so the running clock never
    // glitches.
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    return true;
}
#endif

void PinsTms370Cx5x::resetPins() {
    // The CPU is about to be reset: a glitch on CLKIN is harmless.
    assert_reset();
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
#ifdef TUNE_CLOCK
    _clock.begin(_clockInput);
#else
    _clock.begin(clkin_hz);
#endif
    _clock.start();
    resetCpu();
    _regs->save();
    _regs->setIp(_mems->read16(InstTms370::VEC_RESET));
}

void PinsTms370Cx5x::resetCpu() {
    Cycles::reset();
    assert_reset();
    delayNanoseconds(reset_ns);
    negate_reset();
    // #RESET is open drain; the CPU may hold it low a while longer.
    const uint32_t start = ARM_DWT_CYCCNT;
    while (reset_asserted()) {
        if (ARM_DWT_CYCCNT - start >= stall_cycles) {
            cli.println("?halt: #RESET stuck low");
            return;
        }
    }
    _regs->reset();
}

// The CPU is held in the cycle prepared here until completeCycle().
Signals *PinsTms370Cx5x::prepareCycle() {
    if (!wait_eds(true, stall_cycles))
        return nullptr;
    auto s = SignalsTms370Cx5x::put();
    // assert_debug();
    s->getAddr();
    // #OCF is read after the address, since it may settle after #EDS.
    s->getDirection();
    // negate_debug();
    return s;
}

// Read data is driven before #WAIT is negated and released after #EDS
// rises. Write data is valid by a SYSCLK fall after #EDS falls.
Signals *PinsTms370Cx5x::completeCycle(Signals *_s) {
    auto s = static_cast<SignalsTms370Cx5x *>(_s);
    const auto read = s->read();
    // A held ring reuses this slot, and next() clears its capture flag
    const auto store = !read && s->writeMemory();
    if (read) {
        if (s->readMemory())
            s->data = _mems->read(s->addr);
        s->outData();
    }
    noInterrupts();
    if (!read) {
        wait_sysclk(false, edge_cycles);
        s->getData();
    }
    wait_sysclk(true, edge_cycles);
    // toggle_debug();
    negate_wait();
    wait_eds(false, edge_cycles);
    if (read)
        s->inputMode();
    assert_wait();
    // toggle_debug();
    interrupts();
    if (store)
        _mems->write(s->addr, s->data);
    Cycles::next();
    return s;
}

// Serves cycles up to the next fetch, and holds the CPU there.
bool PinsTms370Cx5x::pauseCpu() {
    for (auto s = prepareCycle(); s != nullptr; s = prepareCycle()) {
        if (s->fetch())
            return true;
        completeCycle(s);
    }
    return false;
}

void PinsTms370Cx5x::injectReads(const uint8_t *data, uint_fast8_t len) {
    uint_fast8_t inj = 0;
    while (inj < len) {
        auto s = prepareCycle();
        if (s == nullptr)
            return no_bus_cycle();
        if (inj < len)
            s->inject(data[inj]);
        completeCycle(s);
        if (s->read())
            inj++;
    }
    if (!pauseCpu())
        no_bus_cycle();
}

uint16_t PinsTms370Cx5x::captureWrites(
        const uint8_t *data, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    uint16_t addr = 0;
    uint_fast8_t inj = 0;
    uint_fast8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (s == nullptr) {
            no_bus_cycle();
            return addr;
        }
        if (inj < len)
            s->inject(data[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->read() && inj < len) {
            if (inj++ == 0)
                addr = s->addr;
        } else if (s->write() && cap < max) {
            buf[cap++] = s->data;
        }
    }
    if (!pauseCpu())
        no_bus_cycle();
    return addr;
}

void PinsTms370Cx5x::loop() {
    _regs->restore();
    Cycles::reset();
    while (true) {
        auto s = prepareCycle();
        if (s == nullptr)
            return no_bus_cycle();
        if (s->fetch()) {
            if (haltSwitch()) {
            stop:
                Cycles::Hold hold;
                takePendingInterrupt(s);
                _regs->save();
                Cycles::discard(s);
                return;
            }
            const auto inst = _mems->read(s->addr);
            if (inst == InstTms370::TRAP15 &&
                    _mems->read16(InstTms370::VEC_TRAP15) ==
                            InstTms370::VEC_TRAP15) {
                goto stop;
            }
            // IDLE stops the bus, and the halt switch with it.
            if (inst == InstTms370::IDLE)
                goto stop;
            s->inject(inst);
        }
        completeCycle(s);
        _devs->loop();
    }
}

// JMP $ runs without an interrupt this many times before a save: one
// pending at the halt is taken at the first boundary, and one recognized a
// boundary late at the second.
constexpr auto park_jumps = 2;
// Bus cycles for a JMP $, or an interrupt's vector reads, to reach the next
// fetch; far fewer are needed.
constexpr auto park_cycles = 32;

void PinsTms370Cx5x::takePendingInterrupt(Signals *fetch) {
    // An interrupt pending at a boundary aborts that fetch and reads its
    // vector next, so it is taken here rather than in the middle of the
    // save; the save then starts at its handler's fetch. Otherwise JMP to
    // itself leaves PC as it was.
    const auto pc = fetch->addr;
    auto s = fetch;
    for (auto jumps = 0; jumps < park_jumps; ++jumps) {
        completeCycle(s->inject(InstTms370::JMP));
        s = prepareCycle();
        for (auto n = 0; s == nullptr || !s->fetch(); ++n) {
            if (s == nullptr || n >= park_cycles) {
                cli.println("?halt: no fetch");
                return;
            }
            if (s->read() && s->addr == pc + 1U)
                s->inject(InstTms370::JMP_HERE);
            completeCycle(s);
            s = prepareCycle();
        }
        if (s->addr != pc)
            return;  // an interrupt's handler
    }
}

void PinsTms370Cx5x::run() {
    saveBreakInsts();
    startRunTimer();
    loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
}

bool PinsTms370Cx5x::rawStep() {
    const auto inst = _mems->read(_regs->nextIp());
    if (inst == InstTms370::TRAP15 || inst == InstTms370::IDLE)
        return false;
    auto s = prepareCycle();
    if (s == nullptr) {
        no_bus_cycle();
        return false;
    }
    // An interrupt enabled by the restore fetches its handler instead.
    completeCycle(s->inject(_mems->read(s->addr)));
    s->clearInject();
    if (pauseCpu())
        return true;
    no_bus_cycle();
    return false;
}

bool PinsTms370Cx5x::step(bool show) {
    _regs->restore();
    if (show)
        Cycles::reset();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsTms370Cx5x::assertInt(uint8_t name) {
    switch (name) {
    case tms370::INTR_INT1:
        digitalWriteFast(PIN_INT1, LOW);
        break;
    case tms370::INTR_INT2:
        digitalWriteFast(PIN_INT2, LOW);
        break;
    case tms370::INTR_INT3:
        digitalWriteFast(PIN_INT3, LOW);
        break;
    }
}

void PinsTms370Cx5x::negateInt(uint8_t name) {
    switch (name) {
    case tms370::INTR_INT1:
        digitalWriteFast(PIN_INT1, HIGH);
        break;
    case tms370::INTR_INT2:
        digitalWriteFast(PIN_INT2, HIGH);
        break;
    case tms370::INTR_INT3:
        digitalWriteFast(PIN_INT3, HIGH);
        break;
    }
}

}  // namespace tms370cx5x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

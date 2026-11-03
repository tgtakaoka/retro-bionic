#include "pins_i80186.h"
#include "debugger.h"
#include "devs_i8086.h"
#include "digital_bus.h"
#include "inst_i8086.h"
#include "mems_i8086.h"
#include "regs_i8086.h"
#include "signals_i8086.h"

namespace debugger {
namespace i8086 {

// clang-format off
/**
 * 80C186XL bus cycle in queue status mode (80C186XL datasheet, Figures 6
 * and 7), as the BionicN80C186 and BionicN80C187 boards run it.
 *
 * The CPU is static, and the Teensy is its clock: it toggles X1, and
 * CLKOUT, X1/2, follows one of X1's edges. T-states start at CLKOUT's
 * falling edge. #RD held low at reset selects queue status mode: ALE and
 * #WR carry QS0 and QS1, and the Teensy tells the cycles from #S2-#S0, as
 * the 8288 would. ARDY stays high: the Teensy stops the clock instead.
 *
 *         |  T4   |     T1      |     T2      |   T3/Tw     |     T4      |
 *         \_______      \_______      \_______      \_______      \______
 * CLKOUT          /       |      /       |      /       |      /       |
 *         _______  ____________________________________  _______________
 * #S2-#S0 _______><____________status___________________>   passive
 *                   __________
 * AD      ---------X___addr___>--------------<_____read data______>------
 *                     ^              ^               ^         ^
 *                   address      write data,       passive   read data
 *                   read         or bus floated    seen      released
 *
 * After each rising edge the Teensy reads QS1:QS0 and #S2-#S0 in one read
 * of GPIO9: the queue status sample, once a CLKOUT period, and the status
 * of a cycle about to start (TCHQSV, TCHSV). The status goes passive at
 * the falling edge of T3, or of the last wait state (User's Manual 3.4.1),
 * so the next falling edge is T4's, where the CPU takes read data. Wait
 * states the chip selects insert need nothing more.
 *
 * Accesses to the Peripheral Control Block, at I/O FF00H, show on the bus
 * but the CPU serves them itself: the Teensy only watches. So are the
 * 80C187's reads on the BionicN80C187 board, whose 74AHC139 makes NPRD and
 * NPWR from #NPS, #DEN and DT/#R. A16-A19 carry no segment in T2 (S3-S5
 * are low).
 */
// clang-format on

namespace {

// The 80C186XL20's timing (80C186XL datasheet); CLKOUT follows X1 by
// TCICO, and a port read takes some 20ns more.
constexpr auto x1_ns = 25;  // TCLCK, TCHCK 10ns, as X1 high at 20 MHz
constexpr auto clkout_ns = 17;                    // TCICO
constexpr auto read_ns = 20;                      // the port read
constexpr auto status_ns = clkout_ns + 27 + read_ns;   // TCHQSV; TCHSV 25
constexpr auto addr_ns = clkout_ns + 27 + read_ns;     // TCLAV
constexpr auto data_ns = clkout_ns + 27 + read_ns;     // TCLDV; TCLAZ 20
constexpr auto passive_ns = clkout_ns + 25 + read_ns;  // TCLSH
constexpr auto hold_ns = clkout_ns + 3;                // TCLDX
// The 80C187's cycles, past what the clock gives them: its read data
// comes 50ns after NPRD (t28), and its strobes keep 69ns apart (t26).
constexpr auto npx_data_ns = 30;
constexpr auto npx_gap_ns = 40;
// CLKOUT periods with no bus cycle before the CPU counts as halted: far
// longer than its longest instruction, or an 80C187 instruction it waits
// for.
constexpr uint32_t no_status_clocks = 4096;
// The Peripheral Control Block, where reset leaves it.
constexpr uint32_t PCB_BASE = 0xFF00;
// The 80C187's I/O ports, 00F8H-00FFH.
constexpr uint32_t NPX_BASE = 0x00F8;

const uint8_t PINS_LOW[] = {
        PIN_X1,
        PIN_NMI,
        PIN_INT0,
        PIN_HOLD,
        PIN_RDQSMD,  // queue status mode
        PIN_RES,
};

const uint8_t PINS_HIGH[] = {
        PIN_ARDY,  // no wait state of its own
};

const uint8_t PINS_INPUT[] = {
        PIN_QS0,
        PIN_QS1,
        PIN_BHE,
        PIN_ALE2,
        PIN_CLKOUT,
        PIN_DEN,
        PIN_HLDA,
        PIN_A16S,
        PIN_A17S,
        PIN_A18S,
        PIN_A19S,
        PIN_AD0,
        PIN_AD1,
        PIN_AD2,
        PIN_AD3,
        PIN_AD4,
        PIN_AD5,
        PIN_AD6,
        PIN_AD7,
        PIN_AD8,
        PIN_AD9,
        PIN_AD10,
        PIN_AD11,
        PIN_AD12,
        PIN_AD13,
        PIN_AD14,
        PIN_AD15,
};

// The status floats in bus hold; pulled up, it reads passive.
const uint8_t PINS_PULLUP[] = {
        PIN_S0,
        PIN_S1,
        PIN_S2,
};

// X1's level whose edge moves CLKOUT, found at reset, and its level now.
uint8_t x1_edge = HIGH;
uint8_t x1_level = LOW;

// One CLKOUT edge: X1 leaves, then takes, the level that moves it.
inline void clkout_edge() {
    digitalWriteFast(PIN_X1, !x1_edge);
    delayNanoseconds(x1_ns);
    digitalWriteFast(PIN_X1, x1_edge);
    x1_level = x1_edge;
}

// Finds the X1 edge that moves CLKOUT, and leaves CLKOUT high.
bool find_edge() {
    for (auto i = 0; i < 4; ++i) {
        const auto clkout = digitalReadFast(PIN_CLKOUT);
        x1_level = !x1_level;
        digitalWriteFast(PIN_X1, x1_level);
        delayNanoseconds(x1_ns + clkout_ns);
        if (digitalReadFast(PIN_CLKOUT) != clkout) {
            x1_edge = x1_level;
            if (clkout) {
                clkout_edge();
                delayNanoseconds(clkout_ns);
            }
            return true;
        }
    }
    return false;
}

void negate_reset() {
    digitalWriteFast(PIN_RES, HIGH);
}

// #S2-#S0 as a number, from GPIO9 as read.
inline uint8_t status_of(uint32_t p9) {
    return (busValue(CNTL, p9) >> 2) & 7;
}

inline bool passive(uint32_t p9) {
    return status_of(p9) == Signals::STATUS_PASSIVE;
}

// NMI takes a rising edge, latched; negate once its vector was read.
void assert_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

}  // namespace

PinsI80186::PinsI80186(bool npx) : PinsI8086Base(npx, QsTracker::N80C186) {
    _devs = new DevsI8080();
    _mems = new MemsI8086(npx ? "80C187" : "off");
    _regs = new RegsI8086(this);
}

#ifdef TUNE_CLOCK
// K 91, 93 and 94 as on the P8086; there is no rate to set.
bool PinsI80186::setClockInput(uint32_t hz) {
    const auto n = hz / 1000;
    if (n == 91) {  // DIAG: the ring raw, for a host replay
        dumpQueue();
    } else if (n == 93) {  // DIAG: the backtrace again, and its QS check
        if (_qsMode == QS_RAW || !_qs.frozen() || !printExecuted(true))
            printCycles();
    } else if (n == 94) {  // DIAG: backtrace by QS pairs, QS0, or raw
        _qsMode = _qsMode == QS_PAIR   ? QS_ZERO
                  : _qsMode == QS_ZERO ? QS_RAW
                                       : QS_PAIR;
    } else {
        return false;
    }
    return true;
}
#endif

// CLKOUT falls: a T-state starts.
void PinsI80186::fall() {
    clkout_edge();
}

// CLKOUT rises; then the queue status and the status are read, and the
// sample goes to the ring.
void PinsI80186::rise() {
    delayNanoseconds(x1_ns);
    clkout_edge();
    delayNanoseconds(status_ns);
    _p9 = PIN(CNTL);
    ++_clocks;
    _qs.push(busValue(CNTL, _p9) & 3);
}

// Reset starts at FFFF0H; the CPU waits in that fetch, the registers are
// read there.
void PinsI80186::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    // The 80C187's BUSY, high through its reset, puts the CPU in Enhanced
    // Mode; without one, #TEST low keeps Compatible Mode and WAIT falls
    // through.
    if (_fpu) {
        pinMode(PIN_TEST, INPUT);
    } else {
        pinMode(PIN_TEST, OUTPUT);
        digitalWriteFast(PIN_TEST, LOW);
    }
    x1_level = LOW;
    _qs.begin();

    // #RES low 4 clocks at least.
    for (auto i = 0; i < 16; ++i) {
        fall();
        rise();
    }
    if (!find_edge())
        cli.println("?CLKOUT");
    Cycles::reset();
    negate_reset();
    _p9 = PIN(CNTL);
    parkAtReset(prepareCycle());
}

bool PinsI80186::pcb(const Signals *s) const {
    return s->io() && !s->intAck() && !s->halt() &&
           (s->addr & 0xFF00) == PCB_BASE;
}

bool PinsI80186::npx(const Signals *s) const {
    return _fpu && s->io() && !s->intAck() && !s->halt() &&
           (s->addr & 0xFFF8) == NPX_BASE;
}

// Clocks on to the next bus cycle and stops in its T2, its address and
// status read, and a write's data valid. Null when the CPU shows none:
// halted.
Signals *PinsI80186::prepareCycle() {
    for (uint32_t n = 0; passive(_p9); ++n) {
        if (n >= no_status_clocks)
            return nullptr;
        fall();
        rise();
    }
    // T4 or an idle state, the status showing; on to T1's address.
    const auto seen = _clocks;
    const auto status = status_of(_p9);
    fall();
    delayNanoseconds(addr_ns);
    // One read of each port: the address and #BHE.
    const uint32_t p6 = PIN(AD);
    const uint32_t p7 = PIN(AH);
    auto s = Signals::put();
    s->setAddr(false, busValue(AH, p7) | busValue(AD, p6),
            ((p7 >> BHE_BIT) & 1) == 0);
    s->setStatus(status);
    s->clear();  // neither injected nor captured yet
    _seenAt[s->pos()] = seen;
    // No segment in S3-S6: a fetch, and a read of the debugger's vectors,
    // go through CS.
    const auto cs = status == Signals::STATUS_CODE ||
                    (status == Signals::STATUS_READ &&
                            InstI8086::vector(s->addr));
    s->setSegment(cs ? SEG_CS : SEG_DS);
    // T2: a write's data shows, or the CPU lets the bus go for a read's.
    rise();
    fall();
    delayNanoseconds(data_ns);
    return s;
}

Signals *PinsI80186::completeCycle(Signals *s) {
    const auto own = pcb(s);
    const auto fpu = npx(s);
    const auto drive = (s->read() || s->intAck()) && !own && !fpu;
    if (drive) {
        if (s->readMemory())
            retrieveData(s);
        s->outData();
    } else if (s->write()) {
        s->getData();
        if (!own && s->writeMemory())
            storeData(s);
    }
    // On to T3, or the last wait state: the status goes passive at its
    // falling edge.
    for (uint32_t n = 0; n < no_status_clocks; ++n) {
        rise();
        fall();
        delayNanoseconds(passive_ns);
        if (passive(PIN(CNTL)))
            break;
    }
    if (fpu)
        delayNanoseconds(npx_data_ns);
    // T4: the CPU takes read data at its falling edge.
    rise();
    fall();
    delayNanoseconds(hold_ns);
    if (drive)
        s->inputMode();
    if (fpu)
        delayNanoseconds(npx_gap_ns);
    // The next cycle's status may show after T4's rising edge.
    rise();
    Cycles::next();
    return s;
}

bool PinsI80186::parkAtReset(Signals *s) {
    const auto reset =
            InstI8086::physical(InstI8086::RESET_CS, InstI8086::RESET_IP);
    if (s == nullptr || !s->read() || !s->memory() || s->addr != reset) {
        cli.print("?reset: ");
        if (s)
            s->print();
        else
            cli.println();
        return false;
    }
    auto regs = this->regs<RegsI8086>();
    regs->parkAt(InstI8086::RESET_CS, InstI8086::RESET_IP);
    regs->save();
    return true;
}

// The clock stops with the CPU: the latest sample is the latest clock's.
void PinsI80186::freezeQs() {
    _qs.freeze(_clocks);
}

void PinsI80186::assertNmi() {
    assert_nmi();
}

void PinsI80186::negateNmi() {
    negate_nmi();
}

// Type 2 raises NMI, an edge the CPU latches and the trap negates; any
// other type raises INT0, level-triggered in cascade mode, high while a
// device's vector() is pending: the CPU reads the vector in its INTA
// cycles.
void PinsI80186::assertInt(uint8_t name) {
    if (name == InstI8086::VEC_NMI) {
        assert_nmi();
    } else {
        digitalWriteFast(PIN_INT0, HIGH);
    }
}

void PinsI80186::negateInt(uint8_t name) {
    if (name != InstI8086::VEC_NMI)
        digitalWriteFast(PIN_INT0, _devs->vector() ? HIGH : LOW);
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

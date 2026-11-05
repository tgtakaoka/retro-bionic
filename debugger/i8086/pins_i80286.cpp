#include "pins_i80286.h"
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
 * 80C286 bus cycle (M80C286 datasheet, Major Cycle Timing), as the
 * BionicN80C286 board runs it.
 *
 * The CPU is static, and the Teensy is its CLK. Each processor clock is
 * two CLK periods, phase 1 and phase 2, each starting at a falling edge.
 * The clock may stop only low in phase 2 (Figure 31), so every wait, and
 * every return to the debugger, is in a phase 2.
 *
 *          |    Ts     |    Tc     |  next Ts  |
 *          | p1  | p2  | p1  | p2  | p1  | p2  |
 *         \   /\   /\   /\   /\   /\   /\   /
 * CLK      \_/  \_/  \_/  \_/  \_/  \_/  \_/
 *          ____         ____________
 * #S1/#S0      \_______/            \_____
 *         _____X_________________X____
 * A0-A23  _____X___address_______X_next
 *                     ____________________
 * D0-D15  -----------<_write data_________>
 *                         <_read data_>
 *         _________________       ________
 * #READY                   \_____/
 *                  ^     ^       ^
 *              stop, read  READY   data and READY
 *              address     low     taken, next status
 *
 * The status goes active after the falling edge that starts Ts phase 1,
 * so after each phase 1 edge the Teensy reads #S1, #S0, M/#IO, #BHE and
 * COD/#INTA in one read of GPIO9. It stops in Ts phase 2 and steps the
 * four muxes through A0-A23; the address is valid from the phase 2
 * before Ts and through Tc phase 1, and a write's data from t14 into Ts
 * phase 2. Read data goes out in Tc phase 1, after the previous write's
 * data has floated (t15). #READY is held low from Tc phase 1 to 25ns
 * past the edge that ends Tc, so it meets its setup and hold whichever
 * of Tc's falling edges samples it, and the CPU takes read data at the
 * edge that ends Tc, where the next cycle's Ts may start (pipelined).
 */
// clang-format on

namespace {

// The N80C286-12's CLK (24 MHz: LOW and HIGH 12ns at least) and the
// delays after a falling edge (M80C286 datasheet, A.C. Characteristics);
// a port read takes some 20ns more.
constexpr auto clk_ns = 25;               // CLK HIGH, and LOW
constexpr auto read_ns = 20;              // the port read
constexpr auto status_ns = 22 + read_ns;  // t12a1, status active delay
constexpr auto ready_ns = 26;             // t10, READY setup
constexpr auto hold_ns = 25;              // t11 READY hold; t9 is 8ns
// The muxes' select to output, through the level shifters.
constexpr auto asel_ns = 40;
// The 74HCT259's #LE pulse and its data setup.
constexpr auto latch_ns = 30;
// Processor clocks with no bus cycle before the CPU counts as halted.
constexpr uint32_t no_status_clocks = 4096;
// RESET high for 16 CLKs at least; the first bus cycle comes some 38
// CLKs after it falls.
constexpr auto reset_clocks = 32;
constexpr auto first_cycle_clks = 400;

// The 74HCT259's outputs.
constexpr uint8_t Q_HOLD = 0;
constexpr uint8_t Q_NMI = 1;
constexpr uint8_t Q_INTR = 2;
// The BionicN80287XL's #NPS1; nothing on the BionicN80C286.
constexpr uint8_t Q_NPS1 = 3;

const uint8_t PINS_LOW[] = {
        PIN_CLK,
        PIN_ASEL0,
        PIN_ASEL1,
        PIN_LATCH_D,
};

const uint8_t PINS_HIGH[] = {
        PIN_RESET,
        PIN_READY,
        PIN_LATCH_LE,
};

const uint8_t PINS_INPUT[] = {
        PIN_A16S,  // P30-P33, the muxes' low outputs
        PIN_A17S,
        PIN_A18S,
        PIN_A19S,
        PIN_MUX4,
        PIN_MUX5,
        PIN_MUX6,
        PIN_MUX7,
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

// The status floats in bus hold; pulled up, it reads inactive.
const uint8_t PINS_PULLUP[] = {
        PIN_S0,
        PIN_S1,
        PIN_MIO,
        PIN_BHE,
        PIN_CODINTA,
};

inline void clk_rise() {
    digitalWriteFast(PIN_CLK, HIGH);
    delayNanoseconds(clk_ns);
}

// Each edge holds CLK at its level for its minimum and more, so no two
// edges come closer, whatever follows.
inline void clk_fall() {
    digitalWriteFast(PIN_CLK, LOW);
    delayNanoseconds(clk_ns);
}

// The bits of GPIO9 as busValue(CNTL) gives them.
constexpr uint32_t S0 = 0x01;
constexpr uint32_t S1 = 0x02;
constexpr uint32_t MIO = 0x04;
constexpr uint32_t BHE = 0x08;
constexpr uint32_t CODINTA = 0x10;

// #S1 or #S0 low: a Ts.
inline bool status_active(uint32_t p9) {
    return (busValue(CNTL, p9) & (S1 | S0)) != (S1 | S0);
}

// COD/#INTA, M/#IO, #S1 and #S0 (Bus Cycle Status Definition) as the
// 8086's #S2-#S0; what will not occur reads passive.
uint8_t status_of(uint32_t p9) {
    static constexpr uint8_t STATUS[16] = {
            0,
            7,
            7,
            7,  // 00xx: interrupt acknowledge
            3,
            5,
            6,
            7,  // 01xx: halt or shutdown, read, write memory
            7,
            1,
            2,
            7,  // 10xx: read, write I/O
            7,
            4,
            7,
            7,  // 11xx: instruction read
    };
    const auto v = busValue(CNTL, p9);
    const auto i =
            ((v & CODINTA) ? 8 : 0) | ((v & MIO) ? 4 : 0) | (v & (S1 | S0));
    return STATUS[i];
}

// One nibble of each mux, P30-P33 and P34-P37.
inline uint32_t mux_low(uint32_t p7) {
    return (p7 >> MUXL_BIT) & 0xF;
}

inline uint32_t mux_high(uint32_t p7) {
    return (p7 >> MUXH_BIT) & 0xF;
}

// The muxes go through 00, 01, 11 and 10, one select changing per step,
// and back to 00; the CPU moves only on our clock edges, so the address
// holds meanwhile. 00 has A0-A3 and A16-A19, 01 A4-A7 and A20-A23, 11
// A8-A11, and 10 A12-A15 (and #LOCK, HLDA and #PEACK).
uint32_t read_address() {
    const uint32_t sel00 = PIN(MUX);
    digitalWriteFast(PIN_ASEL0, HIGH);  // 01
    delayNanoseconds(asel_ns);
    const uint32_t sel01 = PIN(MUX);
    digitalWriteFast(PIN_ASEL1, HIGH);  // 11
    delayNanoseconds(asel_ns);
    const uint32_t sel11 = PIN(MUX);
    digitalWriteFast(PIN_ASEL0, LOW);  // 10
    delayNanoseconds(asel_ns);
    const uint32_t sel10 = PIN(MUX);
    digitalWriteFast(PIN_ASEL1, LOW);  // 00
    return mux_low(sel00) | (mux_low(sel01) << 4) | (mux_low(sel11) << 8) |
           (mux_low(sel10) << 12) | (mux_high(sel00) << 16) |
           (mux_high(sel01) << 20);
}

// One output of the 74HCT259: its address on the selects, D, then a #LE
// pulse; the selects go back to the muxes' 00.
void write_latch(uint8_t q, uint8_t level) {
    digitalWriteFast(PIN_ASEL0, (q & 1) ? HIGH : LOW);
    digitalWriteFast(PIN_ASEL1, (q & 2) ? HIGH : LOW);
    digitalWriteFast(PIN_LATCH_D, level);
    delayNanoseconds(latch_ns);
    digitalWriteFast(PIN_LATCH_LE, LOW);
    delayNanoseconds(latch_ns);
    digitalWriteFast(PIN_LATCH_LE, HIGH);
    delayNanoseconds(latch_ns);
    digitalWriteFast(PIN_ASEL0, LOW);
    digitalWriteFast(PIN_ASEL1, LOW);
}

}  // namespace

PinsI80286::PinsI80286()
    : PinsI8086Base(false, QsTracker::N80C286, InstI8086::ADDR_MASK_24) {
    _devs = new DevsI8080();
    _mems = new MemsI8086("off", InstI8086::ADDR_MASK_24);
    _regs = new RegsI8086(this);
}

// From a phase 2: phase 1 starts, its status is read, and the clock stops
// low in phase 2.
void PinsI80286::clock() {
    clk_rise();
    clk_fall();
    delayNanoseconds(status_ns);
    _p9 = PIN(CNTL);
    clk_rise();
    clk_fall();
}

// After RESET the phase is unknown until the first bus cycle: the edge
// after which the status first reads active starts Ts phase 1. Ends
// stopped in Ts phase 2.
bool PinsI80286::syncPhase() {
    for (auto i = 0; i < first_cycle_clks; ++i) {
        clk_rise();
        clk_fall();
        delayNanoseconds(status_ns);
        _p9 = PIN(CNTL);
        if (status_active(_p9)) {
            clk_rise();
            clk_fall();
            return true;
        }
    }
    return false;
}

// RESET high puts every output in a known state; the 74HCT259 has no
// reset of its own, so HOLD, NMI and INTR go low before it falls.
void PinsI80286::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    write_latch(Q_HOLD, LOW);
    write_latch(Q_NMI, LOW);
    write_latch(Q_INTR, LOW);
    write_latch(Q_NPS1, HIGH);

    for (auto i = 0; i < reset_clocks; ++i) {
        clk_rise();
        clk_fall();
    }
    // RESET falls synchronous to CLK: well before the next falling edge.
    digitalWriteFast(PIN_RESET, LOW);
    Cycles::reset();
    if (!syncPhase()) {
        cli.println("?no bus cycle after RESET");
        return;
    }
    parkAtReset(prepareCycle());
}

// Clocks on to the next bus cycle and stops in its Ts phase 2, its
// address and status read, and a write's data valid. Null when the CPU
// shows none: halted.
Signals *PinsI80286::prepareCycle() {
    for (uint32_t n = 0; !status_active(_p9); ++n) {
        if (n >= no_status_clocks)
            return nullptr;
        clock();
    }
    const auto status = status_of(_p9);
    const auto bhe = (busValue(CNTL, _p9) & BHE) == 0;
    auto s = Signals::put();
    s->setAddr(false, read_address() & _addrMask, bhe);
    s->setWide();
    s->setStatus(status);
    s->clear();  // neither injected nor captured yet
    // A fetch shows COD/#INTA high; a read of the debugger's vectors goes
    // through CS too.
    const auto cs =
            status == Signals::STATUS_CODE ||
            (status == Signals::STATUS_READ && InstI8086::vector(s->addr));
    s->setSegment(cs ? SEG_CS : SEG_DS);
    // A write's data is valid t14 (40ns) into Ts phase 2: the mux steps
    // took longer.
    return s;
}

Signals *PinsI80286::completeCycle(Signals *s) {
    const auto drive = s->read() || s->intAck();
    if (drive) {
        if (s->readMemory())
            retrieveData(s);
    } else if (s->write()) {
        s->getData();
        if (s->writeMemory())
            storeData(s);
    }
    // Tc phase 1: the previous write's data has floated.
    clk_rise();
    clk_fall();
    digitalWriteFast(PIN_READY, LOW);
    if (drive)
        s->outData();
    delayNanoseconds(ready_ns);
    // Tc phase 2.
    clk_rise();
    clk_fall();
    // Tc ends: the CPU takes READY and read data; the next Ts may start.
    clk_rise();
    clk_fall();
    delayNanoseconds(hold_ns);
    digitalWriteFast(PIN_READY, HIGH);
    if (drive)
        s->inputMode();
    delayNanoseconds(status_ns - hold_ns);
    _p9 = PIN(CNTL);
    clk_rise();
    clk_fall();
    Cycles::next();
    return s;
}

// Reset fetches at FFFFF0H, CS based at FF0000H until a far jump reloads
// it. The debugger gives it JMP F000:FFF0, to the 8086's reset address,
// and parks there; from there on CS is CS*16.
bool PinsI80286::parkAtReset(Signals *s) {
    const auto reset = InstI8086::RESET_286;
    if (s == nullptr || !s->code() || s->addr != reset) {
        cli.print("?reset: ");
        if (s)
            s->print();
        else
            cli.println();
        return false;
    }
    static constexpr uint8_t JMP[] = {
            InstI8086::JMPF,
            uint8_t(InstI8086::RESET_IP_286),
            uint8_t(InstI8086::RESET_IP_286 >> 8),
            uint8_t(InstI8086::RESET_CS_286),
            uint8_t(InstI8086::RESET_CS_286 >> 8),
    };
    const auto park =
            physical(InstI8086::RESET_CS_286, InstI8086::RESET_IP_286);
    uint32_t org = reset;
    overlay().clear();
    execInst(JMP, sizeof(JMP), org, park);
    if (org != park) {
        cli.println("?reset: no far jump");
        return false;
    }
    auto regs = this->regs<RegsI8086>();
    regs->parkAt(InstI8086::RESET_CS_286, InstI8086::RESET_IP_286);
    regs->save();
    return true;
}

// NMI takes a rising edge after 4 CLKs low, latched; negate once its
// vector was read.
void PinsI80286::assertNmi() {
    write_latch(Q_NMI, HIGH);
}

void PinsI80286::negateNmi() {
    write_latch(Q_NMI, LOW);
}

// Type 2 raises NMI; any other type raises INTR, high while a device's
// vector() is pending: the CPU reads the vector in the second of its two
// INTA cycles.
void PinsI80286::assertInt(uint8_t name) {
    if (name == InstI8086::VEC_NMI) {
        assertNmi();
    } else {
        write_latch(Q_INTR, HIGH);
    }
}

void PinsI80286::negateInt(uint8_t name) {
    if (name != InstI8086::VEC_NMI)
        write_latch(Q_INTR, _devs->vector() ? HIGH : LOW);
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

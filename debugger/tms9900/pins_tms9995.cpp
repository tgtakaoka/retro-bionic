#include "pins_tms9995.h"
#include "debugger.h"
#include "devs_tms9900.h"
#include "mems_tms9995.h"
#include "regs_tms9995.h"
#include "signals_tms9995.h"

namespace debugger {
namespace tms9995 {

// clang-format off
/**
 * TMS9995 bus cycle
 *    PHI    |  1  |  2  |  3  |  4  |  1  |
 *         __    __    __    __    __    __
 *  CLKIN |  |__|  |__|  |__|  |__|  |__|  |__|
 *        ___\     \     \ __________\
 * CLKOUT     |___________|           |________
 *        __________                        ___
 * #MEMEN __________\______________________/___
 *        ________________             ________
 *  #DBIN                 \___________/
 *        ________________             ________
 *    #WE                 \___________/
 *        ________________ ____________________
 *   DATA ________________X___________R________
 *                         ___________
 *    IAQ ________________/           \________
 *        _____________________________________
 *  READY ___________________________/  \______
 */
// clang-format on

// fext: min 8 MHz, max 12.1 MHz
//  tc1: min  82 ns ; 1/fext
//  tc2: min 328 ns ; 4tc1 cycle of CLKOUT
//  tWH: min  41 ns ; CLKIN high pulse width
//  tWL: min  41 ns ; CLKIN low pulse width
//  td2: max  1 tc1 ; CLKOUT- to valid address
//  td3: max  1 tc1 ; CLKOUT- to #MEMEN-
//  td4: max  1 tc1 ; CLKOUT- to #MEMEN+
// tsu1: min 100 ns ; READY setup to CLKOUT-
//  tdz: max  1 tc1+60 ns; CLKOUT- to HiZ data
// tsu2: min  65 ns ; DATA valid setup to CLKOUT-
//  td5: max  40 ns ; CLKOUT+ to #DBIN-
//  td6: max  50 ns ; CLKOUT- to #DBIN+
//  td7: max  40 ns ; CLKOUT+ to IAQ+
//  td8: max  50 ns ; CLKOUT- to IAQ-
//  td9: max  40 ns ; CLKOUT+ to DATA out
// td10: max  40 ns ; CLKOUT+ to #WE-
// td11: max  50 ns ; CLKOUT- to #WE+

namespace {

constexpr auto clkin_lo_ns = 40;      // 41
constexpr auto clkin_hi_ns = 30;      // 41
constexpr auto clkin_lo_inject = 20;  // 41
constexpr auto clkin_lo_get = 5;      // 41
constexpr auto clkin_hi_capture = 5;  // 41

const uint8_t PINS_LOW[] = {
        PIN_CLKIN,
        PIN_RESET,
        PIN_READY,
        PIN_CRUIN,
};

const uint8_t PINS_HIGH[] = {
        PIN_NMI,
        PIN_INT1,
        PIN_INT4,
        PIN_HOLD,
};

const uint8_t PINS_INPUT[] = {
        PIN_CLKOUT,
        PIN_MEMEN,
        PIN_DBIN,
        PIN_WE,
        PIN_IAQ,
        PIN_D7,
        PIN_D6,
        PIN_D5,
        PIN_D4,
        PIN_D3,
        PIN_D2,
        PIN_D1,
        PIN_D0,
        PIN_AL15,
        PIN_AL14,
        PIN_AL13,
        PIN_AL12,
        PIN_AL11,
        PIN_AL10,
        PIN_AL9,
        PIN_AL8,
        PIN_AM7,
        PIN_AM6,
        PIN_AM5,
        PIN_AM4,
        PIN_AH3,
        PIN_AH2,
        PIN_AH1,
        PIN_AH0,
};

inline void clkin_lo() {
    digitalWriteFast(PIN_CLKIN, LOW);
}

inline void clkin_hi() {
    digitalWriteFast(PIN_CLKIN, HIGH);
}

auto signal_clkout() {
    return digitalReadFast(PIN_CLKOUT);
}

void clkin_cycle_hi() {
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
    clkin_hi();
}

void clkin_cycle() {
    clkin_cycle_hi();
    delayNanoseconds(clkin_hi_ns);
}

void system_cycle() {
    clkin_cycle();
    clkin_cycle();
    clkin_cycle();
    clkin_cycle();
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

auto ready_asserted() {
    return digitalReadFast(PIN_READY) != LOW;
}

}  // namespace

PinsTms9995::PinsTms9995() {
    _devs = new tms9900::DevsTms9900();
    _mems = new MemsTms9995(this, _devs);
    _regs = new RegsTms9995(this, _mems);
}

void PinsTms9995::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    // Synchronize CLKIN to CLKOUT
    while (signal_clkout() == LOW)
        clkin_cycle();
    while (signal_clkout() != LOW)
        clkin_cycle();
    // phi1

    for (auto i = 0; i < 10; ++i)
        system_cycle();
    negate_reset();

    Signals::resetCycles();
    pauseCycle();
    _regs->save();
    _regs->reset();
}

tms9900::Signals *PinsTms9995::prepareCycle() const {
    auto s = Signals::put();
    // phi2
    clkin_cycle();
    // phi3
    clkin_lo();
    s->getAddress();
    clkin_hi();
    return s;
}

tms9900::Signals *PinsTms9995::completeCycle(tms9900::Signals *_s) const {
    auto s = static_cast<Signals *>(_s);
    if (ready_asserted()) {
        s->getControl();
        if (s->read()) {
            // phi4
            clkin_lo();
            if (s->readMemory()) {
                s->data = _mems->read(s->addr);
            } else {
                delayNanoseconds(clkin_lo_inject);
            }
            clkin_hi();
            s->outData();
            // phi1
            clkin_lo();
            Signals::nextCycle();
            clkin_hi();
            s->inputMode();
        } else if (s->write()) {
            // phi4
            clkin_lo();
            delayNanoseconds(clkin_lo_get);
            s->getData();
            clkin_hi();
            if (s->writeMemory()) {
                _mems->write(s->addr, s->data);
            } else {
                delayNanoseconds(clkin_hi_capture);
            }
            // phi1
            clkin_lo();
            Signals::nextCycle();
            clkin_hi();
        } else {
            goto nobus;
        }
    } else {
    nobus:
        // phi4
        clkin_cycle();
        // phi1
        clkin_cycle_hi();
    }
    return s;
}

tms9900::Signals *PinsTms9995::resumeCycle(uint16_t) const {
    auto s = Signals::put();
    s->getAddress();
    s->getControl();
    assert_ready();
    return s;
}

void PinsTms9995::cycle() {
    completeCycle(resumeCycle());
    pauseCycle();
}

bool PinsTms9995::is_internal(uint16_t addr) const {
    if (addr < 0xF000)
        return false;
    if (addr < 0xF0FC)
        return true;
    return addr >= 0xFFFC;
}

const uint8_t JMP_6[] = {0x10, 0xFC};  // JMP $-6
const uint8_t ZERO[] = {0, 0};

uint8_t PinsTms9995::internal_read(uint16_t addr) {
    // MOVB @s, @d; I:I:s:s:R:d:d:i:i:W
    const uint8_t MOVB1[] = {0xD8, 0x20, hi(addr), lo(addr)};
    injectReads(MOVB1, sizeof(MOVB1));  // MOVB @addr,
    cycle();                            // one read
    injectReads(ZERO, sizeof(ZERO));    // , @0
    injectReads(JMP_6, sizeof(JMP_6));  // JMP $-6
    uint8_t data;
    captureWrites(&data, sizeof(data));
    cycle();  // finish JMP
    return data;
}

void PinsTms9995::internal_write(uint16_t addr, uint8_t data) {
    // MOVB @s, @d; I:I:s:s:R:d:d:i:i:W
    const uint8_t MOVB[] = {0xD8, 0x20, 0, 0, data, hi(addr), lo(addr)};
    injectReads(MOVB, sizeof(MOVB));    // MOVB @0, @addr
    injectReads(JMP_6, sizeof(JMP_6));  // JMP $-6
    cycle();                            // one write
    cycle();                            // finish JMP
}

uint16_t PinsTms9995::internal_read16(uint16_t addr) {
    // MOV @s, @d; I:I:s:s:R:d:i:i:W:w
    const uint8_t MOV1[] = {0xC8, 0x20, hi(addr), lo(addr)};
    uint8_t buf[sizeof(uint16_t)];
    injectReads(MOV1, sizeof(MOV1));    // MOV @addr,
    cycle();                            // one read
    injectReads(ZERO, sizeof(ZERO));    // , @0
    injectReads(JMP_6, sizeof(JMP_6));  // JMP $-6
    captureWrites(buf, sizeof(buf));    // capture W:w
    cycle();                            // finish JMP
    return (buf[0] << 8) | buf[1];      // Regs::be16(buf);
}

void PinsTms9995::internal_write16(uint16_t addr, uint16_t data) {
    // MOV @s, @d; I:I:s:s:R::r:d:d:i:i:W
    const uint8_t MOV[] = {
            0xC8, 0x20, 0, 0, hi(data), lo(data), hi(addr), lo(addr)};
    injectReads(MOV, sizeof(MOV));      // MOVB @0, @addr
    injectReads(JMP_6, sizeof(JMP_6));  // JMP $-6
    cycle();                            // one write
    cycle();                            // finish JMP
}

void PinsTms9995::assertInt(uint8_t name_) {
    const auto name = static_cast<tms9900::IntrName>(name_);
    if (name == tms9900::INTR_NMI)
        digitalWriteFast(PIN_NMI, LOW);
    if (name == tms9900::INTR_INT1)
        digitalWriteFast(PIN_INT1, LOW);
    if (name == tms9900::INTR_INT4)
        digitalWriteFast(PIN_INT4, LOW);
}

void PinsTms9995::negateInt(uint8_t name_) {
    const auto name = static_cast<tms9900::IntrName>(name_);
    if (name == tms9900::INTR_NMI)
        digitalWriteFast(PIN_NMI, HIGH);
    if (name == tms9900::INTR_INT1)
        digitalWriteFast(PIN_INT1, HIGH);
    if (name == tms9900::INTR_INT4)
        digitalWriteFast(PIN_INT4, HIGH);
}

}  // namespace tms9995
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

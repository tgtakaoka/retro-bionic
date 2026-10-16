#include "pins_z88.h"
#include "debugger.h"
#include "devs_z8.h"
#include "inst_z88.h"
#include "mems_z88.h"
#include "regs_z88.h"
#include "z88_uart_handler.h"

namespace debugger {
namespace z88 {

// clang-format off
/**
 * Z8 External Bus cycle
 *        -|-----T1----|-----T2----|----T3-----|-----T1----|-----T2----|-----T3----|-
 *          __    __    __    __    __    __    __    __    __    __    __    __    _
 * XTAL1  _|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
 *         \     \     \  \              \     \     \     \     \           \     \
 *        __|     |_____|__|______________|_____|     |_____|____|____________|_____|
 *   #AS    |_____|     |  |              |     |_____|     |    |            |     |
 *        __|___________|  |              |_____|___________|____|            |_____|
 *   #DS    |           |__|______________|     |           |    |____________|     |
 *        __|______________|____________________|           |                       |
 *  R/#W  __|              |                    |___________|_______________________/
 *          |______________|         _____      |___________|_______________________|
 *    P1  --<____A7-A0_____>--------<D7-D0>-----<_A7-A0_____X__________D7-D0________X
 *        __|___________________________________|___________________________________|
 *    P0  __X______________A15-A8_______________X_______________A15-A8______________X
 *        __|___________________________________|___________________________________|
 *   #DM  __X___________________________________X___________________________________X
 */
// clang-format on
namespace {
//   TpC: min  50 ns; XTAL1 cycle (20MHz)
//   TwC: min  25 ns; XTAL1 width
//  TwAS: min  65 ns; #AS width
//   TdA: min  35 ns; Address valid to #AS+
//  TdAS: min  65 ns; #AS+ to Address hold
// TwDSR: min 125 ns; #DS read width
// TdDSR: max  80 ns; #DS- to read data
// TwDSW: min  65 ns; #DS write width
//  TdDW: min  10 ns; write data to #DS-
//  TdDS: min   0 ns; #DS+ to write data hold

// XTAL1~10MHz
constexpr auto xtal1_hi_ns = 20;  // 50 ns
constexpr auto xtal1_lo_ns = 0;   // 50 ns

inline void xtal1_hi() {
    digitalWriteFast(PIN_XTAL1, HIGH);
    delayNanoseconds(xtal1_hi_ns);
}

inline void xtal1_lo() {
    digitalWriteFast(PIN_XTAL1, LOW);
    if (xtal1_lo_ns)
        delayNanoseconds(xtal1_lo_ns);
}

void z88_cycle(SerialHandler *serial) {
    xtal1_hi();
#if defined(ENABLE_SERIAL_HANDLER)
    serial->loop();
#endif
    xtal1_lo();
}

}  // namespace

PinsZ88::PinsZ88()
    : PinsZ8(false, z88_cycle,
#if defined(ENABLE_SERIAL_HANDLER)
              new Z88UartHandler(),
#else
              nullptr,
#endif
              Inst) {
    auto regs = new RegsZ88(this);
    _regs = regs;
    _devs = new z8::DevsZ8(_serial);
    _mems = new MemsZ88(regs, _devs);
}

#if defined(Z88_FAST_INTERRUPT)
void PinsZ88::startFast(uint16_t ip, bool enabled, bool fis) {
    _ip = ip;
    _fastEnabled = enabled;
    _fis = fis;
    armFast();
}

void PinsZ88::armFast() {
    _fastFetch = (_fastEnabled && !_fis) ? _ip : NO_FAST;
}

void PinsZ88::takeFast(z8::Signals *s) {
    const auto discarded = s->prev(2);
    discarded->fetch() = false;
    s->prev()->fetch() = false;
    _ip = discarded->addr;
    _fis = true;
    armFast();
}

// Follows the immediate writes to IP and SYM; a fast IRET is just the
// opcode and the byte after it.
uint8_t PinsZ88::fastCycles(uint16_t addr, uint8_t inst, uint8_t cycles) {
    constexpr uint8_t LDW_IM = 0xC6;  // LDW RR,#IML
    constexpr uint8_t LD_IM = 0xE6;   // LD R,#IM
    constexpr uint8_t IRET = 0xBF;
    if (inst == IRET && _fis) {
        _ip = addr + 1;
        _fis = false;
        armFast();
        return 2;
    }
    if (inst != LDW_IM && inst != LD_IM)
        return cycles;
    const auto dst = _mems->read_byte(addr + 1);
    const auto im = _mems->read_byte(addr + 2);
    if (inst == LDW_IM && dst == RegsZ88::IPH) {
        _ip = uint16(im, _mems->read_byte(addr + 3));
    } else if (inst == LD_IM && dst == RegsZ88::IPH) {
        _ip = uint16(im, lo(_ip));
    } else if (inst == LD_IM && dst == RegsZ88::IPL) {
        _ip = uint16(hi(_ip), im);
    } else if (inst == LD_IM && dst == RegsZ88::SYM) {
        _fastEnabled = im & RegsZ88::SYM_FAST;
    }
    armFast();
    return cycles;
}
#endif

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

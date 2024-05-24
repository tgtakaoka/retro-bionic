#include "pins_z88.h"
#include "debugger.h"
#include "devs_z88.h"
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
    UartH.loop();
}

inline void xtal1_lo() {
    digitalWriteFast(PIN_XTAL1, LOW);
}

void cycle() {
    xtal1_hi();
    delayNanoseconds(xtal1_hi_ns);
    xtal1_lo();
    if (xtal1_lo_ns)
        delayNanoseconds(xtal1_lo_ns);
}

}  // namespace

struct PinsZ8 Pins {
    false, cycle, Regs, Inst, Memory, Devs,
};

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

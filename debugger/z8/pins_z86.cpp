#include "pins_z86.h"
#include "debugger.h"
#include "devs_z8.h"
#include "inst_z86.h"
#include "mems_z86.h"
#include "regs_z86.h"
#include "z86_sio_handler.h"

namespace debugger {
namespace z86 {

// clang-format off
/**
 * Z8 External Bus cycle
 *       -|-----T1----|-----T2----|----T3-----|-----T1----|-----T2----|-----T3----|-
 *       _    __    __    __    __    __    __    __    __    __    __    __    __
 * XTAL1  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |_
 *        \     \     \  \              \     \     \     \     \           \     \
 *       __|     |_____|__|______________|_____|     |_____|_____|__________|______|
 *   #AS   |_____|     |  |              |     |_____|     |     |          |      |
 *       __|___________|  |              |_____|___________|_____|          |______|
 *   #DS   |           |__|______________|     |           |     |__________|      |
 *         |______________|         _____       ___________|_______________________
 *    P1 --<____A7-A0_____>--------<D7-D0>-----<___A7-A0___X________D0-D7__________X
 *         |___________________________________|                                   |
 *  R/#W --|                                   |___________________________________/
 *          ___________________________________ ___________________________________
 *    P0 --X_____________A15-A8________________X______________A15-A8_______________X
 */
// clang-format on
// The XTAL1 cycle is PinsZ8::Traits<VAR_Z86>'s.
//   TpC: min  62.5 ns; XTAL1 cycle (16MHz)
//   TwC: min  25 ns; XTAL1 width
//  TwAS: min  40 ns; #AS width
//   TdA: min  25 ns; Address valid to #AS+
//  TdAS: min  35 ns; #AS+ to Address hold
// TdAS(DS): min  45 ns; #AS+ to #DS-
// TwDSR: min 135 ns; #DS read width
// TdDSR: max  75 ns; #DS- to read data
//  TdDI: min  60 ns; #DS+ to data input setup
// TwDSW: min  80 ns; #DS write width
//  TdDW: min  25 ns; write data to #DS-
//  TdDS: min  35 ns; #DS+ to write data hold

PinsZ86::PinsZ86()
    : PinsZ8(z8::VAR_Z86,
#if defined(ENABLE_SERIAL_HANDLER)
              new Z86SioHandler(),
#else
              nullptr,
#endif
              Inst) {
    auto regs = new RegsZ86(this);
    _regs = regs;
    auto devs = new z8::DevsZ8(_serial);
    _devs = devs;
    _mems = new MemsZ86(regs, devs);
}

}  // namespace z86
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

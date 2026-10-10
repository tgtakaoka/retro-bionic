#ifndef __DEVS_MC68HC16_H__
#define __DEVS_MC68HC16_H__

#include "mc6800/devs_mc6800.h"

namespace debugger {
namespace mc68hc16 {

// The MC6850 ACIA below the on-chip register block: SIM space the chip
// does not implement goes out on the bus (User's Manual 3.6).
constexpr uint32_t ACIA_BASE_HC16 = 0xFFE00;

}  // namespace mc68hc16
}  // namespace debugger
#endif /* __DEVS_MC68HC16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __DEVS_H16_H__
#define __DEVS_H16_H__

#include "mc6800/devs_mc6800.h"

namespace debugger {
namespace h16 {

// The MC6850 ACIA below the park origin, clear of the on-chip RAM and
// I/O at their reset places.
constexpr uint32_t ACIA_BASE_H16 = 0xFFDF00;

}  // namespace h16
}  // namespace debugger
#endif /* __DEVS_H16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

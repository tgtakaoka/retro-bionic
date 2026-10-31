#ifndef __DEVS_H8500_H__
#define __DEVS_H8500_H__

#include "mc6800/devs_mc6800.h"

namespace debugger {
namespace h8500 {

// The MC6850 ACIA in page 0, external on both chips: below the H8/536's
// on-chip RAM (H'F680-FE7F) and register field.
constexpr uint32_t ACIA_BASE_H8500 = 0xF000;

}  // namespace h8500
}  // namespace debugger
#endif /* __DEVS_H8500_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

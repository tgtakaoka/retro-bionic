#ifndef __DEVS_NS32016_H__
#define __DEVS_NS32016_H__

#include "mc6800/devs_mc6800.h"

namespace debugger {
namespace ns32016 {

// The MC6850 ACIA near the top, clear of the interrupt acknowledge
// addresses FFFE00/FFFF00: control/status at the even byte (AD0-AD7),
// data at the odd one (AD8-AD15).
constexpr uint32_t ACIA_BASE_NS32016 = 0xFFFC00;

}  // namespace ns32016
}  // namespace debugger
#endif /* __DEVS_NS32016_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

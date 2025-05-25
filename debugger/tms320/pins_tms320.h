#ifndef __PINS_TMS320_H__
#define __PINS_TMS320_H__

#include "pins.h"

namespace debugger {
namespace tms320 {

struct PinsTms320 : Pins {
    virtual uint16_t injectRead(uint16_t data) = 0;
    virtual uint16_t captureWrite() = 0;
};

}  // namespace tms320
}  // namespace debugger
#endif /* __PINS_TMS3201X_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

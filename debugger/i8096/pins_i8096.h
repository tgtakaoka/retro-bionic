#ifndef __PINS_I8096_H__
#define __PINS_I8096_H__

#include "pins.h"

namespace debugger {
namespace i8096 {

struct PinsI8096 : Pins {
    uint16_t injectRead(uint8_t data) { return injectReads(&data, 1); }
    virtual uint16_t injectReads(const uint8_t *data, uint_fast8_t len) = 0;
    virtual uint16_t captureWrites(uint8_t *data, uint_fast8_t len) = 0;
};

}  // namespace i8096
}  // namespace debugger
#endif /* __PINS_I8096_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __PINS_I8080_BASE_H__
#define __PINS_I8080_BASE_H__

#include "pins.h"

namespace debugger {
namespace i8080 {

struct PinsI8080Base : Pins {
    void setBreakInst(uint32_t addr) const override;

    void execInst(const uint8_t *inst, uint_fast8_t len);
    virtual uint16_t captureWrites(const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf, uint_fast8_t max) = 0;
};

}  // namespace i8080
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __PINS_TMS370_H__
#define __PINS_TMS370_H__

#define CNTL_EDS 0x1 /* CNTL0 */
#define CNTL_RW 0x2  /* CNTL1 */
#define CNTL_OCF 0x4 /* CNTL2 */

#include "pins.h"

namespace debugger {
namespace tms370 {

enum IntrName : uint8_t {
    INTR_INT1 = 1,
    INTR_INT2 = 2,
    INTR_INT3 = 3,
};

struct PinsTms370 : Pins {
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;

    virtual void injectReads(const uint8_t *data, uint_fast8_t len) = 0;
    virtual uint16_t captureWrites(const uint8_t *data, uint_fast8_t len,
            uint8_t *buf, uint_fast8_t max) = 0;

protected:
    void disassembleCycles();
};

}  // namespace tms370
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

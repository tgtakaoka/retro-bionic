#ifndef __PINS_TMS9900_H__
#define __PINS_TMS9900_H__

#define CNTL_MEMEN 0x1 /* CNTL0; active low */
#define CNTL_DBIN 0x2  /* CNTL1; active high */
#define CNTL_WE 0x4    /* CNTL2; active low */
#define CNTL_IAQ 0x8   /* CNTL3; active high */
#define CNTL_16BIT 0x10
#define PIN_READY 31   /* P8.22; active high */

#include "pins.h"
#include "signals_tms9900.h"

namespace debugger {
namespace tms9900 {

enum IntrName : uint8_t {
    INTR_NMI = 0xFC,
    INTR_INT1 = 0x04,
    INTR_INT2 = 0x08,
    INTR_INT3 = 0x0C,
    INTR_INT4 = 0x10,
    INTR_INT5 = 0x14,
    INTR_INT6 = 0x18,
    INTR_INT7 = 0x1C,
    INTR_INT8 = 0x20,
    INTR_INT9 = 0x24,
    INTR_INT10 = 0x28,
    INTR_INT11 = 0x2C,
    INTR_INT12 = 0x30,
    INTR_INT13 = 0x34,
    INTR_INT14 = 0x38,
    INTR_INT15 = 0x3C,
};

struct PinsTms9900 : Pins {
    void printCycles() override;

    void idle() override;
    bool step(bool show) override;
    void run() override;
    void setBreakInst(uint32_t addr) const override;

    void injectReads(const uint8_t *data, uint8_t len);
    void captureReads(uint8_t *buf, uint8_t len) {
        captureCycles(buf, len, false);
    }
    void captureWrites(uint8_t *buf, uint8_t len) {
        captureCycles(buf, len, true);
    }
    virtual bool is_internal(uint16_t) const { return false; }
    virtual uint8_t internal_read(uint16_t addr) { return 0; }
    virtual void internal_write(uint16_t addr, uint8_t data) {}

protected:
    bool rawStep();
    void loop();

    virtual Signals *pauseCycle();
    virtual Signals *resumeCycle(uint16_t pc = 0) const = 0;
    virtual Signals *prepareCycle() const = 0;
    virtual Signals *completeCycle(Signals *s) const = 0;
    void captureCycles(uint8_t *buf, uint8_t len, bool write);
    void suspend(uint16_t pc);

    void disassembleCycles();
};

}  // namespace tms9900
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

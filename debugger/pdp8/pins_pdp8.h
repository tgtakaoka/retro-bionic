#ifndef __PINS_PDP8_H__
#define __PINS_PDP8_H__

#include "pins.h"
#include "signals_pdp8.h"

#define PIN_INTREQ 9 /* P7.11 */

#define IOC_C2 1     /* IOC0 */
#define IOC_C1 2     /* IOC1 */
#define IOC_C0 4     /* IOC2 */
#define IOC_SKIP 8   /* IOC3 */
#define DIR_WRITE 1  /* DIR0 */
#define DIR_READ 2   /* DIR1 */
#define DIR_IFETCH 4 /* DIR2 */
#define CNTL_MEM 1   /* CNTL0 */
#define CNTL_DEV 2   /* CNTL1 */
#define CNTL_CP 4    /* CNTL2 */
#define CNTL_15BIT 0x20

namespace debugger {
namespace pdp8 {

struct PinsPdp8 : Pins {
    bool step(bool show) override;
    void run() override;
    void idle() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;

    void captureWrites(const uint16_t *data, uint_fast8_t len, uint16_t *buf,
            uint_fast8_t max) const;
    void injectReads(const uint16_t *data, uint_fast8_t len) const;

protected:
    Mems *_cpmems;

    PinsPdp8() {}

    virtual pdp8::Signals *resumeCycle(uint16_t pc = 0) const = 0;
    virtual pdp8::Signals *prepareCycle() const = 0;
    // Returns nullptr if found HLT instruction
    virtual pdp8::Signals *completeCycle(Signals *signals) const = 0;

    void loop();
    void suspend();
    bool rawStep();
    void disassembleCycles() const;
};

}  // namespace pdp8
}  // namespace debugger
#endif /* __PINS_PDP8_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __PINS_P8095BH_H__
#define __PINS_P8095BH_H__

#define PORT_HSO 7   /* GPIO7 */
#define HSO_gp 0     /* P7.00-P7.03 */
#define HSO_gm 0xF   /* P7.00-P7.03 */
#define HSO_vp 0     /* A0-A3 */
#define PIN_HSO0 10  /* P7.00 */
#define PIN_HSO1 12  /* P7.01 */
#define PIN_HSO2 11  /* P7.02 */
#define PIN_HSO3 13  /* P7.03 */
#define PORT_HSI 7   /* P7.16-P7.19 */
#define HSI_gp 16    /* P7.16-P7.19 */
#define HSI_gm 0xF   /* P7.16-P7.19 */
#define HSI_vp 4     /* A4-A7 */
#define PIN_HSI0 8   /* P7.16 */
#define PIN_HSI1 7   /* P7.17 */
#define PIN_HSI2 36  /* P7.18 */
#define PIN_HSI3 37  /* P7.19 */
#define PIN_TXD 0    /* P6.03 */
#define PIN_RXD 1    /* P6.02 */
#define PIN_PWM 5    /* P9.08 */
#define PIN_XTAL1 29 /* P9.31 */
#define PIN_ACH4 6   /* P7.10 */
#define PIN_ACH5 9   /* P7.11 */
#define PIN_ACH6 32  /* P7.12 */
// #define PIN_ACH7 35   /* P7.28 */

#include "inst_i8096.h"
#include "pins_i8096.h"
#include "signals_p8095bh.h"

namespace debugger {
namespace p8095bh {

struct PinsP8095BH final : i8096::PinsI8096 {
    PinsP8095BH();

    void idle() override;
    bool step(bool show) override;
    void run() override;

    void printCycles() override { printCycles(nullptr); }
    void assertInt(uint8_t name = 0) override;
    void negateInt(uint8_t name = 0) override;
    void setBreakInst(uint32_t addr) const override;

    uint16_t injectReads(
            const uint8_t *data, uint_fast8_t len, bool idle = false) override;
    uint16_t captureWrites(uint8_t *data, uint_fast8_t len) override;

private:
    bool _idle;
    Signals _idleSignals;

    void resetPins() override;
    bool rawStep(bool show);
    Signals *loop();

    Signals *prepareCycle();
    Signals *completeCycle(Signals *s);

    Signals *jumpHere(uint_fast8_t len = 4, bool idle = false);
    uint16_t jumpTarget(uint16_t next, uint_fast8_t opc) const;
    void handleTrap(Signals *s, uint16_t vector, bool breakTrap);

    void printCycles(const Signals *end);
    bool matchAll(Signals *begin, const Signals *end);
    const Signals *findFetch(Signals *begin, const Signals *end);
    void disassembleCycles();
};

}  // namespace p8095bh
}  // namespace debugger
#endif /* __PINS_P8095BH_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

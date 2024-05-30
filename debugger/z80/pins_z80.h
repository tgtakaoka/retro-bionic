#ifndef __PINS_Z80_H__
#define __PINS_Z80_H__

#define PIN_INT 6    /* P7.10 */
#define PIN_NMI 9    /* P7.11 */
#define PIN_WAIT 32  /* P7.12 */

#include "pins.h"
#include "signals_z80.h"

namespace debugger {
namespace z80 {

using z80::Signals;
struct RegsZ80;

struct PinsZ80 : Pins {
    PinsZ80(RegsZ80 &regs) : Pins(), _regs(regs) {}

    void reset() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void printCycles() override;

    void execInst(const uint8_t *inst, uint8_t len);
    void captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

protected:
    RegsZ80 &_regs;

    void setBreakInst(uint32_t addr) const override;

    virtual Signals *prepareCycle() const;
    virtual Signals *completeCycle(Signals *signals) const;
    virtual Signals *prepareWait() const;
    Signals *resumeCycle(uint16_t pc) const;

    Signals *inject(uint8_t data) const;
    void loop();
    void suspend();
    bool rawStep();
    void execute(const uint8_t *inst, uint8_t len, uint16_t *addr, uint8_t *buf,
            uint8_t max);

    void disassembleCycles();
};

extern struct PinsZ80 Pins;

}  // namespace z80
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

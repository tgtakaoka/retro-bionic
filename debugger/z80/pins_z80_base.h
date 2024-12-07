#ifndef __PINS_Z80_BASE_H__
#define __PINS_Z80_BASE_H__

#include "pins.h"
#include "signals.h"

namespace debugger {
namespace z80 {

struct MemsZ80;
struct RegsZ80;

struct PinsZ80Base : Pins {
    void execInst(const uint8_t *inst, uint8_t len);
    void captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);
    void setBreakInst(uint32_t addr) const override;

protected:
    PinsZ80Base(RegsZ80 &regs, MemsZ80 &mems) : _regs(regs), _mems(mems) {}

    RegsZ80 &_regs;
    MemsZ80 &_mems;

    virtual void execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max) = 0;

};

}  // namespace z80
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __REGS_MC68HC11_H__
#define __REGS_MC68HC11_H__

#include "mc6800/regs_mc6800.h"

namespace debugger {
namespace mc68hc11 {

struct Mc68hc11Init;

struct RegsMc68hc11 final : mc6800::RegsMc6800 {
    RegsMc68hc11(mc6800::PinsMc6800Base *pins, Mc68hc11Init &init);

    const char *cpu() const override;

    void print() const override;
    void save() override;
    void restore() override;
    uint8_t contextLength() const override { return 9; }
    uint16_t capture(const mc6800::Signals *stack, bool step) override;

    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

    uint8_t internal_read(uint16_t addr) const override;
    void internal_write(uint16_t addr, uint8_t data) const override;

    SoftwareType checkSoftwareType() override;

protected:
    Mc68hc11Init &_init;
    uint16_t _y;

    void _d(uint16_t d) {
        _b = lo(d);
        _a = hi(d);
    }
};

}  // namespace mc68hc11
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

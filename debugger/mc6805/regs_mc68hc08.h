#ifndef __REGS_MC68HC08_H__
#define __REGS_MC68HC08_H__

#include "pins_mc6805.h"
#include "regs_mc6805.h"

namespace debugger {
namespace mc68hc08 {

using mc6805::Signals;

struct RegsMc68HC08 final : mc6805::RegsMc6805 {
    RegsMc68HC08(mc6805::PinsMc6805 *pins);

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;

    void captureExtra(uint16_t pc) override;

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    uint8_t _h;

    void saveH();
    bool isHC08() const override { return true; }
};

}  // namespace mc68hc08
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

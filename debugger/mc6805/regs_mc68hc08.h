#ifndef __REGS_MC68HC08_H__
#define __REGS_MC68HC08_H__

#include "pins_mc6805.h"
#include "regs_mc6805.h"

namespace debugger {
namespace mc68hc08 {

struct RegsMc68HC08 final : mc6805::RegsMc6805 {
    RegsMc68HC08(mc6805::PinsMc6805 *pins);

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;

    //    bool saveContext(const Signals *frame);

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    uint8_t internal_read(uint16_t addr) const;
    void internal_write(uint16_t addr, uint8_t data, bool irv = true) const;

private:
    uint8_t _h;
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

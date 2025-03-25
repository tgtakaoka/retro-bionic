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

private:
    uint8_t _h;

    bool is6805() const override { return false; }
    void load_hx(uint16_t hx) const;    
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

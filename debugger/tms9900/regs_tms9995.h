#ifndef __REGS_TMS9995_H__
#define __REGS_TMS9995_H__

#include "regs_tms9900.h"

namespace debugger {
namespace tms9995 {

struct PinsTms9995;

struct RegsTms9995 final : tms9900::RegsTms9900 {
    RegsTms9995(PinsTms9995 *pins, Mems *mems);

    const char *cpu() const override;

    void reset() override;
    void save() override;
    void restore() override;

    void breakPoint() override;
    uint16_t read_reg(uint8_t i) const override;
    void write_reg(uint8_t i, uint16_t data) const override;
};

}  // namespace tms9995
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __INST_Z88_H__
#define __INST_Z88_H__

#include "inst_z8.h"

namespace debugger {
namespace z88 {

struct InstZ88 final : z8::InstZ8 {
    uint8_t instLen(uint8_t inst) const override;
    uint8_t busCycles(uint8_t inst) const override;
    uint8_t breakInst() const override { return WFI; }
    bool isBreak(uint8_t inst) const override { return inst == WFI; }

    static constexpr uint8_t SRP0(uint8_t val) { return ((val & 0xF8) + 2); }
    static constexpr uint8_t SRP1(uint8_t val) { return ((val & 0xF8) + 1); }
    static bool writeOnly(uint8_t rp0, uint8_t rp1, uint8_t num);

    static constexpr uint16_t ORG_RESET = 0x0020;

private:
    static constexpr uint8_t WFI = 0x3F;
};

extern const struct InstZ88 Inst;

}  // namespace z88
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

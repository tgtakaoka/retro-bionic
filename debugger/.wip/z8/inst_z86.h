#ifndef __INST_Z86_H__
#define __INST_Z86_H__

#include "inst_z8.h"

namespace debugger {
namespace z86 {

struct InstZ86 final : z8::InstZ8 {
    uint8_t instLen(uint8_t inst) const override;
    uint8_t busCycles(uint8_t inst) const override;
    uint8_t breakInst() const override { return HALT; }
    bool isBreak(uint8_t inst) const override {
        return inst == STOP || inst == HALT;
    }

    static constexpr uint16_t ORG_RESET = 0x000C;

    static bool writeOnly(uint8_t rp, uint8_t num);

private:
    static constexpr uint8_t STOP = 0x6F;
    static constexpr uint8_t HALT = 0x7F;
};

extern const struct InstZ86 Inst;

}  // namespace z86
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

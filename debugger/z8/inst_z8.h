#ifndef __INST_Z8_H__
#define __INST_Z8_H__

#include <stdint.h>

namespace debugger {
namespace z8 {

struct InstZ8 {
    virtual uint8_t instLen(uint8_t inst) const = 0;
    virtual uint8_t busCycles(uint8_t inst) const = 0;
    virtual uint8_t breakInst() const = 0;
    virtual bool isBreak(uint8_t inst) const = 0;

    static constexpr uint8_t JR = 0x8B;
    static constexpr uint8_t JR_HERE = 0xFE;
    static constexpr uint8_t NOP = 0xFF;
};

}  // namespace z8
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

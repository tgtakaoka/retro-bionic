#ifndef __INST_I8048_H__
#define __INST_I8048_H__

#include <stdint.h>

namespace debugger {
namespace i8048 {

struct InstI8048 final {
    enum CpuFlags : uint8_t {
        F_80C39 = 0x2,
        F_MSM39 = 0x4,
        F_X8048 = 0x8,
        F_I8039 = 0x1,
        F_I80C39 = F_I8039 | F_80C39,
        F_MSM80C39 = F_I8039 | F_80C39 | F_MSM39,
        F_I8048 = F_X8048 | F_I8039,
        F_I80C48 = F_X8048 | F_I8039 | F_80C39,
        F_MSM80C48 = F_X8048 | F_I8039 | F_80C39 | F_MSM39,
    };

    InstI8048(CpuFlags flags) : _flags(flags) {}

    uint8_t instLength(uint8_t inst) const;
    uint8_t busCycles(uint8_t inst) const;

    static constexpr uint8_t NOP = 0x00;
    static constexpr uint8_t HALT = 0x01;
    static constexpr uint8_t JF1 = 0x76;

    static constexpr uint8_t SEL_MB(uint8_t mb) {
        return mb == 0 ? SEL_MB0 : SEL_MB1;
    }

    static constexpr uint8_t JMP(uint16_t addr) {
        return ((addr >> 3) & 0xE0) + 0x04;
    }

    static constexpr uint16_t offset(uint16_t addr) { return addr & 0x7FF; }

    static constexpr uint8_t mb(uint16_t addr) { return (addr >> 11) & 1; }

private:
    const CpuFlags _flags;

    static constexpr uint8_t SEL_MB0 = 0xE5;
    static constexpr uint8_t SEL_MB1 = 0xF5;
};

}  // namespace i8048
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

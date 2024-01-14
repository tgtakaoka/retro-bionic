#ifndef __REGS_MOS6502_H__
#define __REGS_MOS6502_H__

#include "regs.h"

namespace debugger {
namespace mos6502 {

enum SoftwareType : uint8_t {
    SW_MOS6502 = 0,  // HW_MOS6502
    SW_G65SC02 = 1,  // HW_MOS6502
    SW_R65C02 = 2,   // HW_MOS6502
    SW_W65C02S = 3,  // HW_W65C02S
    SW_W65C816 = 4,  // HW_W65C816
    SW_NONE = 5,
};

struct RegsMos6502 final : Regs {
    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void save() override;
    void restore() override;
    void reset() { _type = SW_NONE; }

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

private:
    uint16_t _pc;
    uint16_t _d;
    uint16_t _s;
    uint16_t _x;
    uint16_t _y;
    uint8_t _a;
    uint8_t _b;
    uint8_t _p;
    uint8_t _e;
    uint8_t _pbr;
    uint8_t _dbr;
    SoftwareType _type;

    uint16_t _c() const { return uint16(_b, _a); }
    void _c(uint16_t c) {
        _a = lo(c);
        _b = hi(c);
    }
    static constexpr auto P_M = 0x20;
    static constexpr auto P_X = 0x10;
    void setP(uint8_t value);
    void setE(uint8_t value);
    void checkSoftwareType();
    void save65816();
    void restore65816();

    static void setle16(uint8_t *p, uint16_t v) {
        p[0] = lo(v);
        p[1] = hi(v);
    }
};

extern struct RegsMos6502 Registers;

}  // namespace mos6502
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

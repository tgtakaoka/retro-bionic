#ifndef __REGS_MOS6502_H__
#define __REGS_MOS6502_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace mos6502 {

struct PinsMos6502;
struct MemsMos6502;

struct RegsMos6502 final : Regs {
    RegsMos6502(PinsMos6502 *pins, MemsMos6502 *mems);

    const char *cpu() const override;
    const char *cpuName() const override { return cpu(); }

    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override;
    void setIp(uint32_t pc) override;
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsMos6502 *const _pins;
    MemsMos6502 *const _mems;

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

    uint16_t _c() const { return uint16(_b, _a); }
    void _c(uint16_t c) {
        _a = lo(c);
        _b = hi(c);
    }
    static constexpr auto P_M = 0x20;
    static constexpr auto P_X = 0x10;
    void setP(uint8_t value);
    void setE(uint8_t value);
    void save65816();
    void restore65816();

    mutable CharBuffer _buffer;
};

}  // namespace mos6502
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

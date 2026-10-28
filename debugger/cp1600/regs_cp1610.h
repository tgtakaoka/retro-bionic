#ifndef __REGS_CP1610_H__
#define __REGS_CP1610_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace cp1600 {

struct PinsCp1610;
struct MemsCp1610;

struct RegsCp1610 final : Regs {
    RegsCp1610(PinsCp1610 *pins, MemsCp1610 *mems);

    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _r[PC]; }
    void setIp(uint32_t pc) override { _r[PC] = pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    // The debugger's interrupt pushed |pc| at |sp|.
    void interrupted(uint16_t pc, uint16_t sp);

private:
    PinsCp1610 *const _pins;
    MemsCp1610 *const _mems;

    uint16_t _r[8];
    static constexpr auto SP = 6;
    static constexpr auto PC = 7;
    uint8_t _sw;  // S, Z, OV, C at bit 7-4 as GSWD
    bool _intff;

    mutable CharBuffer _buffer;
};

}  // namespace cp1600
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

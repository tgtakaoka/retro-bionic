#ifndef __REGS_TMS7000_H__
#define __REGS_TMS7000_H__

#include "regs.h"

namespace debugger {
namespace tms7000 {

struct RegsTms7000 final : Regs {
    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void reset();
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void restoreA();
    void setIp(uint16_t addr) { _pc = addr; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

    uint8_t read_internal(uint16_t add);
    void write_internal(uint16_t add, uint8_t data);

private:
    uint8_t _a;
    uint8_t _b;
    uint8_t _sp;
    uint8_t _st;
    uint16_t _pc;

    static constexpr uint16_t A = 0x0000;
    static constexpr uint16_t B = 0x0001;
};

extern struct RegsTms7000 Regs;

}  // namespace tms7000
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

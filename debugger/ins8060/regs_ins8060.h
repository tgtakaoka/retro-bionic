#ifndef __REGS_INS8060_H__
#define __REGS_INS8060_H__

#include "regs.h"
#include "signals_ins8060.h"

namespace debugger {
namespace ins8060 {

struct RegsIns8060 final : Regs {
    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _addr(_pc(), _pc() + 1); }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

private:
    uint8_t _a;
    uint8_t _e;
    uint8_t _s;
    uint16_t _ptr[4];
    uint16_t _pc() const { return _ptr[0]; }

    static uint16_t _addr(uint16_t page, uint16_t offset) {
        return (page & 0xF000) | (offset & 0x0FFF);
    }
};

extern struct RegsIns8060 Regs;

}  // namespace ins8060
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

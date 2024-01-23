#ifndef __REGS_INS8070_H__
#define __REGS_INS8070_H__

#include "inst_ins8070.h"
#include "regs.h"

namespace debugger {
namespace ins8070 {

struct RegsIns8070 final : Regs {
    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc() + 1; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

    uint8_t busCycles(InstIns8070 &inst) const;

    static uint8_t internal_read(uint16_t addr);
    static void internal_write(uint16_t addr, uint8_t data);

private:
    uint8_t _a;
    uint8_t _e;
    uint8_t _s;
    uint16_t _t;
    uint16_t _ptr[4];
    uint16_t _pc() const { return _ptr[0]; }
    uint16_t _sp() const { return _ptr[1]; }
    uint16_t _p2() const { return _ptr[2]; }
    uint16_t _p3() const { return _ptr[3]; }
    uint16_t &_pc() { return _ptr[0]; }
    uint16_t &_sp() { return _ptr[1]; }
    uint16_t &_p2() { return _ptr[2]; }
    uint16_t &_p3() { return _ptr[3]; }
    void _ea(uint16_t ea) {
        _a = lo(ea);
        _e = hi(ea);
    }

    uint16_t effectiveAddr(const InstIns8070 &inst) const;
};

extern struct RegsIns8070 Regs;

}  // namespace ins8070
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

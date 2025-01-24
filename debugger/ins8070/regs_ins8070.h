#ifndef __REGS_INS8070_H__
#define __REGS_INS8070_H__

#include "char_buffer.h"
#include "inst_ins8070.h"
#include "regs.h"

namespace debugger {
namespace ins8070 {

struct PinsIns8070;

struct RegsIns8070 final : Regs {
    RegsIns8070(PinsIns8070 *pins);

    const char *cpu() const override { return cpuName(); }

    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc() + 1; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    uint8_t busCycles(InstIns8070 &inst) const;

    uint8_t internal_read(uint16_t addr) const;
    void internal_write(uint16_t addr, uint8_t data) const;

    uint16_t effectiveAddr(const InstIns8070 &inst, uint8_t opr) const;

private:
    PinsIns8070 *const _pins;

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

    mutable CharBuffer _buffer;
};

}  // namespace ins8070
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

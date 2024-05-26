#ifndef __REGS_MC6805_H__
#define __REGS_MC6805_H__

#include "regs.h"

namespace debugger {
namespace mc6805 {

struct PinsMc6805;
struct MemsMc6805;
struct Signals;

struct RegsMc6805 : Regs {
    RegsMc6805(PinsMc6805 &pins, MemsMc6805 &mems) : _pins(pins), _mems(mems) {}

    void print() const override;
    void save() override;
    void restore() override;

    bool saveContext(const Signals *frame);
    void setIp(uint16_t addr) { _pc = addr; }

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

    uint8_t internal_read(uint16_t addr) const;
    void internal_write(uint16_t addr, uint8_t data) const;

protected:
    PinsMc6805 &_pins;
    MemsMc6805 &_mems;

    uint16_t _sp;
    uint16_t _pc;
    uint8_t _x;
    uint8_t _a;
    uint8_t _cc;
};

}  // namespace mc6805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

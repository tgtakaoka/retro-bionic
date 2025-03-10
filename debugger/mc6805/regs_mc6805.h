#ifndef __REGS_MC6805_H__
#define __REGS_MC6805_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace mc6805 {

struct PinsMc6805;
struct Signals;

struct RegsMc6805 : Regs {
    const char *cpu() const override { return _cpu; }

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;

    bool captureContext(const Signals *frame);
    void setIp(uint32_t addr) override { _pc = addr; }

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    uint8_t internal_read(uint16_t addr, bool branch = true) const;
    void internal_write(uint16_t addr, uint8_t data, bool branch = true) const;

protected:
    RegsMc6805(const char *cpu, PinsMc6805 *pins);

    const char *const _cpu;
    PinsMc6805 *const _pins;

    uint16_t _sp;
    uint16_t _pc;
    uint8_t _x;
    uint8_t _a;
    uint8_t _cc;

    mutable CharBuffer _buffer;

    void bra(int8_t offset) const;
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

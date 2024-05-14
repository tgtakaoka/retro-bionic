#ifndef __REGS_I8048_H__
#define __REGS_I8048_H__

#include "regs.h"

namespace debugger {
namespace i8048 {

struct PinsI8048;

struct RegsI8048 final : Regs {
    RegsI8048(const char *cpu, const char *name, PinsI8048 &pins)
        : _cpu(cpu), _name(name), _pins(pins) {}
    const char *cpu() const override { return _cpu; }
    const char *cpuName() const override { return _name; }

    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

    uint8_t read_internal(uint8_t addr) const;
    void write_internal(uint8_t addr, uint8_t data) const;

private:
    const char *const _cpu;
    const char *const _name;
    PinsI8048 &_pins;

    static constexpr uint8_t cy = 0x80;
    static constexpr uint8_t ac = 0x40;
    static constexpr uint8_t f0 = 0x20;
    static constexpr uint8_t bs = 0x10;
    static constexpr uint8_t sp = 0x07;
    uint8_t _psw;
    void set_psw(uint8_t val);
    void set_r(uint8_t no, uint8_t val) const;
    void set_rb(uint8_t value);
    uint8_t r_base() const { return (_psw & bs) ? 0x18 : 0x00; }
    uint8_t _f1;
    uint8_t _a;
    uint8_t _r[8];
    void save_r();
    void restore_r() const;
    void restore_pc(uint16_t pc, uint8_t mb) const;
    uint8_t _mb;
    uint16_t _pc;
};

extern struct RegsI8048 Regs;

}  // namespace i8048
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __REGS_I8051_H__
#define __REGS_I8051_H__

#include "regs.h"

namespace debugger {
namespace i8051 {

struct RegsI8051 final : Regs {
    const char *cpu() const override { return "8051"; }
    const char *cpuName() const override;

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
    static constexpr uint8_t B = 0xF0;
    static constexpr uint8_t ACC = 0xE0;
    static constexpr uint8_t PSW = 0xD0;
    static constexpr uint8_t SP = 0x81;
    static constexpr uint8_t DPL = 0x82;
    static constexpr uint8_t DPH = 0x83;
    uint16_t _pc;
    uint16_t _dptr;
    uint8_t _sp;
    uint8_t _a;
    uint8_t _b;
    uint8_t _psw;
    void set_psw(uint8_t val);
    uint8_t rs() const { return (_psw >> 3) & 3; }
    void set_rs(uint8_t val);
    uint8_t _r[8];
    uint8_t r_base() const { return rs() * 8; }
    void save_r();
    void set_r(uint8_t num, uint8_t val);
    uint8_t raw_read_internal(uint8_t addr) const;

    static constexpr uint8_t SAVE_A[] = {
            0xF2,  // MOVX @R0, A
    };
};

extern struct RegsI8051 Regs;

}  // namespace i8051
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

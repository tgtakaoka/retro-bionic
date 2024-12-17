#ifndef __REGS_Z86_H__
#define __REGS_Z86_H__

#include "regs_z8.h"

namespace debugger {
namespace z86 {

using z8::RegSpace;
using z8::RegsZ8;

struct RegsZ86 final : RegsZ8 {
    RegsZ86();

    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;

    void reset() override;
    RegSpace parseSpace(const char *space) const override {
        return z8::SET_ONE;
    }
    uint8_t read_reg(uint8_t addr, RegSpace space = z8::SET_ONE) override;
    void write_reg(
            uint8_t addr, uint8_t val, RegSpace space = z8::SET_ONE) override;

    void helpRegisters() const override;

private:
    uint8_t _rp;
    uint8_t save_rp() const;
    void restore_rp(uint8_t rp) const;

    static constexpr uint8_t FLAGS = 0xFC;
    static constexpr uint8_t RP = 0xFD;
    static constexpr uint8_t SPH = 0xFE;
    static constexpr uint8_t SPL = 0xFF;
    static constexpr uint8_t R(uint8_t num) { return 0xE0 + num; }
    void save_sfrs() override;
    void restore_sfrs() override;
    uint8_t save_r(uint8_t num) const override;
    void restore_r(uint8_t num, uint8_t val) const override;
    uint8_t raw_read_reg(uint8_t addr) const;

    void set_flags(uint8_t flags) override;
    void set_rp(uint8_t rp) override;
    void set_sp(uint16_t sp) override;
    virtual void set_r(uint8_t num, uint8_t val) override;
    void update_r(uint8_t addr, uint8_t val);
};

extern struct RegsZ86 Regs;

}  // namespace z86
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

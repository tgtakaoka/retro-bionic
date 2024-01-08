#ifndef __REGS_Z86_H__
#define __REGS_Z86_H__

#include "regs.h"

namespace debugger {
namespace z86 {

struct RegsZ86 final : Regs {
    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void save() override;
    void restore() override;

    void reset();
    uint8_t read_reg(uint8_t addr);
    void write_reg(uint8_t addr, uint8_t val);

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t reg) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

private:
    uint16_t _pc;
    static constexpr auto sfr_base = 252;
    static constexpr auto sfr_flags = sfr_base + 0;
    static constexpr auto sfr_rp = sfr_base + 1;
    static constexpr auto sfr_sph = sfr_base + 2;
    static constexpr auto sfr_spl = sfr_base + 3;
    uint8_t _sfr[4];
    uint8_t _r[16];

    void set_flags(uint8_t val) { set_sfr(sfr_flags, val); }
    void set_sph(uint8_t val) { set_sfr(sfr_sph, val); }
    void set_spl(uint8_t val) { set_sfr(sfr_spl, val); }
    void set_rp(uint8_t val);
    bool in_rp(uint8_t num) const {
        const auto rp_base = get_sfr(sfr_rp) & 0xF0;
        return num >= rp_base && num <= rp_base + 15;
    }
    void set_sfr(uint8_t name, uint8_t val);
    uint8_t get_sfr(uint8_t name) const {
        const auto num = name - sfr_base;
        return _sfr[num];
    }

    void save_r(uint8_t num);
    void restore_r(uint8_t num);
    void set_r(uint8_t num, uint8_t val);
    void set_rr(uint8_t num, uint16_t val) {
        set_r(num, hi(val));
        set_r(num + 1, lo(val));
    }
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

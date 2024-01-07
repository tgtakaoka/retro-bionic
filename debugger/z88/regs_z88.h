#ifndef __REGS_Z88_H__
#define __REGS_Z88_H__

#include "regs.h"

namespace debugger {
namespace z88 {

enum RegSpace : uint8_t {
    SET_NONE,
    SET_ONE,    // %00~%FF
    SET_TWO,    // %C0~%FF
    SET_BANK1,  // %E0-%FF
};

struct RegsZ88 final : Regs {
    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void save() override;
    void restore() override;

    void reset();
    uint8_t read_reg(uint8_t addr, RegSpace space = SET_ONE);
    void write_reg(uint8_t addr, uint8_t val);

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

private:
    uint16_t _pc;
    static constexpr auto sfr_base = 213;
    static constexpr auto sfr_flags = sfr_base + 0;
    static constexpr auto sfr_rp0 = sfr_base + 1;
    static constexpr auto sfr_rp1 = sfr_base + 2;
    static constexpr auto sfr_sph = sfr_base + 3;
    static constexpr auto sfr_spl = sfr_base + 4;
    static constexpr auto sfr_iph = sfr_base + 5;
    static constexpr auto sfr_ipl = sfr_base + 6;
    uint8_t _sfr[7];
    uint8_t _r[16];

    void set_flags(uint8_t val) { set_sfr(sfr_flags, val); }
    void set_rp0(uint8_t val);
    void set_rp1(uint8_t val);
    void set_sp(uint16_t val);
    void set_ip(uint16_t val);
    void set_sfr(uint8_t name, uint8_t val);
    uint8_t get_sfr(uint8_t name) const { return _sfr[name - sfr_base]; }

    void update_r(uint8_t addr, uint8_t val);
    void save_r(uint8_t num);
    void restore_r(uint8_t num);
    void set_r(uint8_t num, uint8_t val);
    void set_rr(uint8_t num, uint16_t val) {
        set_r(num, hi(val));
        set_r(num + 1, lo(val));
    }

    static void switchBank(RegSpace space);
};

extern struct RegsZ88 Regs;

}  // namespace z88
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

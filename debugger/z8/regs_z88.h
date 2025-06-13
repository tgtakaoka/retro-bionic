#ifndef __REGS_Z88_H__
#define __REGS_Z88_H__

#include "regs_z8.h"

namespace debugger {
namespace z88 {

struct PinsZ88;

enum RegSpace : uint8_t {
    SET_NONE,
    SET_BANK0,  // Z88: %00~%FF
    SET_BANK1,  // Z88: %E0-%FF
    SET_TWO,    // Z88: %00~%FF
};

struct RegsZ88 final : z8::RegsZ8 {
    RegsZ88(PinsZ88 *pins);

    const char *cpu() const override;

    void print() const override;

    void reset() override;
    uint_fast8_t read_reg(uint8_t addr, RegSpace space = SET_BANK0);
    void write_reg(uint8_t addr, uint8_t val, RegSpace space = SET_BANK0);

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    uint16_t _ip;
    uint8_t _rp0;
    uint8_t _rp1;
    uint_fast8_t save_rp0() const;
    void restore_rp0(uint_fast8_t rp0) const;

    static constexpr uint8_t FLAGS = 0xD5;
    static constexpr uint8_t RP0 = 0xD6;
    static constexpr uint8_t RP1 = 0xD7;
    static constexpr uint8_t SPH = 0xD8;
    static constexpr uint8_t SPL = 0xD9;
    static constexpr uint8_t IPH = 0xDA;
    static constexpr uint8_t IPL = 0xDB;
    static constexpr uint_fast8_t R(uint_fast8_t num) { return 0xC0 + num; }
    void save_sfrs() override;
    void restore_sfrs() override;
    uint_fast8_t save_r(uint_fast8_t num) const override;
    void restore_r(uint_fast8_t num, uint8_t val) const override;
    uint_fast8_t raw_read_reg(uint8_t addr) const;

    void set_flags(uint_fast8_t flags) override;
    void set_rp(uint_fast8_t rp) override;
    void set_sp(uint16_t sp) override;
    void set_ip(uint16_t ip);
    void set_rp0(uint_fast8_t val);
    void set_rp1(uint_fast8_t val);
    virtual void set_r(uint_fast8_t num, uint_fast8_t val) override;
    void update_r(uint_fast8_t addr, uint_fast8_t val);

    void switchBank(RegSpace space);

    mutable CharBuffer _buffer1;
};

}  // namespace z88
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

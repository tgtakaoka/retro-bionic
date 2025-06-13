#ifndef __REGS_Z86_H__
#define __REGS_Z86_H__

#include "regs_z8.h"

namespace debugger {
namespace z86 {

struct PinsZ86;

struct RegsZ86 final : z8::RegsZ8 {
    RegsZ86(PinsZ86 *pins);

    const char *cpu() const override;

    void print() const override;

    void reset() override;
    uint_fast8_t read_reg(uint_fast8_t addr);
    void write_reg(uint8_t addr, uint8_t val);

    void helpRegisters() const override;

private:
    uint8_t _rp;
    uint_fast8_t save_rp() const;
    void restore_rp(uint8_t rp) const;

    static constexpr uint8_t FLAGS = 0xFC;
    static constexpr uint8_t RP = 0xFD;
    static constexpr uint8_t SPH = 0xFE;
    static constexpr uint8_t SPL = 0xFF;
    static constexpr uint8_t R(uint_fast8_t num) { return 0xE0 + num; }
    void save_sfrs() override;
    void restore_sfrs() override;
    uint_fast8_t save_r(uint_fast8_t num) const override;
    void restore_r(uint_fast8_t num, uint8_t val) const override;
    uint_fast8_t raw_read_reg(uint8_t addr) const;

    void set_flags(uint_fast8_t flags) override;
    void set_rp(uint_fast8_t rp) override;
    void set_sp(uint16_t sp) override;
    virtual void set_r(uint_fast8_t num, uint_fast8_t val) override;
    void update_r(uint_fast8_t addr, uint_fast8_t val);

    mutable CharBuffer _buffer1;
};

}  // namespace z86
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

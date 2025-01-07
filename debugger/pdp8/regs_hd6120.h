#ifndef __REGS_HD6120_H__
#define __REGS_HD6120_H__

#include "char_buffer.h"
#include "regs_pdp8.h"

namespace debugger {
namespace hd6120 {

struct PinsHd6120;

struct RegsHd6120 final : pdp8::RegsPdp8 {
    RegsHd6120(PinsHd6120 *pins);

    const char *cpu() const override;

    void print() const override;
    void save() override;
    void restore() override;
    void breakPoint() override;

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    static constexpr auto REG_SP1 = 5;
    static constexpr auto REG_SP2 = 6;
    static constexpr auto REG_GT = 9;
    static constexpr auto REG_IF = 10;
    static constexpr auto REG_DF = 11;
    uint16_t _sp1;
    uint16_t _sp2;
    static constexpr auto gt = 02000;
    static constexpr auto isf = 00070;
    static constexpr auto dsf = 00007;
    uint_fast8_t _gt() const { return (_flags & gt) >> 10; }
    uint_fast8_t _if() const { return (_flags & isf) >> 3; }
    uint_fast8_t _df() const { return (_flags & dsf) >> 0; }
    void set_gt(uint_fast8_t value) {
        _flags &= ~gt;
        _flags |= value << 10;
    }
    void set_if(uint_fast8_t value) {
        _flags &= ~isf;
        _flags |= value << 3;
    }
    void set_df(uint_fast8_t value) {
        _flags &= ~dsf;
        _flags |= value;
    }

    mutable CharBuffer _buffer;
};

}  // namespace hd6120
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

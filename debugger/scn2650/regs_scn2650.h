#ifndef __REGS_SCN2650_H__
#define __REGS_SCN2650_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace scn2650 {

struct PinsScn2650;

struct RegsScn2650 final : Regs {
    RegsScn2650(PinsScn2650 *pins);

    const char *cpu() const override;

    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsScn2650 *const _pins;

    uint16_t _pc;
    uint8_t _psu;
    uint8_t _psl;
    uint8_t rs() const { return (_psl & 0x10) ? 1 : 0; }
    uint8_t _r0;
    uint8_t _r[/*rs*/ 2][3];

    void setRs(uint8_t rs) const;
    void saveRegs(uint8_t *regs) const;
    void restoreRegs(const uint8_t *regs) const;

    static constexpr uint8_t PPSL = 0x77;
    static constexpr uint8_t CPSL = 0x75;

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
};

}  // namespace scn2650
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

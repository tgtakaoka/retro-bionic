#ifndef __REGS_SCN2650_H__
#define __REGS_SCN2650_H__

#include "regs.h"

namespace debugger {
namespace scn2650 {

struct RegsScn2650 final : Regs {
    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void save() override;
    void restore() override;

    void reset();

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

    uint8_t read_io(uint8_t addr) const;
    void write_io(uint8_t addr, uint8_t data) const;

private:
    uint16_t _pc;
    uint8_t _psu;
    uint8_t _psl;
    uint8_t rs() const { return (_psl & 0x10) ? 1 : 0; }
    uint8_t _r0;
    uint8_t _r[/*rs*/ 2][3];

    static void setRs(uint8_t rs);
    static void saveRegs(uint8_t *regs);
    static void restoreRegs(const uint8_t *regs);

    static constexpr uint8_t PPSL = 0x77;
    static constexpr uint8_t CPSL = 0x75;
};

extern struct RegsScn2650 Regs;

}  // namespace scn2650
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

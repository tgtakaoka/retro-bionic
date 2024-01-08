#ifndef __REGS_CDP1802_H__
#define __REGS_CDP1802_H__

#include "regs.h"

namespace debugger {
namespace cdp1802 {

struct RegsCdp1802 final : Regs {
    const char *cpu() const override;
    const char *cpuName() const override;
    bool is1804() const;

    void print() const override;
    void save() override;
    void restore() override;

    void reset();

    uint32_t nextIp() const override { return _r[_p]; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

private:
    uint8_t _d;
    uint8_t _x;
    uint8_t _p;
    uint8_t _t;
    uint8_t _q;
    uint8_t _df;
    uint8_t _ie;
    uint16_t _r[16];
    bool _dirty[16];
    const char *_cpuType;

    void setCpuType();
};

extern struct RegsCdp1802 Regs;

}  // namespace cdp1802
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

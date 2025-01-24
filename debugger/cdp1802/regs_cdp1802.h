#ifndef __REGS_CDP1802_H__
#define __REGS_CDP1802_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace cdp1802 {

struct PinsCdp1802;

struct RegsCdp1802 final : Regs {
    RegsCdp1802(PinsCdp1802 *pins);

    const char *cpu() const override;
    const char *cpuName() const override;
    bool is1804() const;

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _r[_p]; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsCdp1802 *const _pins;

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

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;
};

}  // namespace cdp1802
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

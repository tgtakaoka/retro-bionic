#ifndef __REGS_MC6809_H__
#define __REGS_MC6809_H__

#include "regs.h"

namespace debugger {
namespace mc6809 {

struct PinsMc6809;
struct Signals;

enum SoftwareType : uint8_t {
    SW_MC6809 = 0,
    SW_HD6309 = 1,
};

struct RegsMc6809 : Regs {
    RegsMc6809(PinsMc6809 *pins) : _pins(pins) {}

    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void reset() const;
    void save() override;
    void restore() override;
    uint8_t contextLength() const;
    uint16_t capture(const Signals *frame, bool step = false);
    void setIp(uint16_t addr) { _pc = addr; }

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

    SoftwareType checkSoftwareType();

protected:
    PinsMc6809 *const _pins;
    SoftwareType _type;
    bool _native6309;

    uint16_t _s;
    uint16_t _pc;
    uint16_t _u;
    uint16_t _y;
    uint16_t _x;
    uint8_t _dp;
    uint8_t _a;
    uint8_t _b;
    uint8_t _cc;
    void _d(uint16_t d) {
        _a = hi(d);
        _b = lo(d);
    }
    uint8_t _e;
    uint8_t _f;
    void _w(uint16_t w) {
        _e = hi(w);
        _f = lo(w);
    }
    uint16_t _v;
    void _q(uint32_t q) {
        _d(hi16(q));
        _w(lo16(q));
    }

    void saveContext(uint8_t *context, uint8_t n, uint16_t sp);
    void loadStack(uint16_t sp) const;
    void loadMode(bool native) const;
    void saveVW();
    void loadVW() const;
};

extern struct RegsMc6809 Regs;

}  // namespace mc6809
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __REGS_MC6800_H__
#define __REGS_MC6800_H__

#include "regs.h"
#include "signals_mc6800.h"

namespace debugger {

enum SoftwareType : uint8_t {
    SW_MC6800 = 0,
    SW_MB8861 = 1,
    SW_MC6801 = 2,
    SW_HD6301 = 3,
    SW_MC68HC11 = 4,
};

namespace mc6800 {

struct PinsMc6800Base;

struct RegsMc6800 : Regs {
    RegsMc6800(PinsMc6800Base *pins) : _pins(pins) {}

    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;
    virtual uint8_t contextLength() const { return 7; }
    virtual uint16_t capture(const Signals *stack, bool step = true);
    void setIp(uint32_t addr) override { _pc = addr; }

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

    virtual SoftwareType checkSoftwareType();

    virtual uint8_t internal_read(uint16_t addr) const { return 0; }
    virtual void internal_write(uint16_t addr, uint8_t data) const {}

protected:
    PinsMc6800Base *_pins;
    SoftwareType _type;

    uint16_t _sp;
    uint16_t _pc;
    uint16_t _x;
    uint8_t _a;
    uint8_t _b;
    uint8_t _cc;

    static const uint8_t LDS_7FFF[3];
    static const uint8_t STAA_8000[3];
};

}  // namespace mc6800
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

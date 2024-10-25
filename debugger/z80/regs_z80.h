#ifndef __REGS_Z80_H__
#define __REGS_Z80_H__

#include "pins_z80_base.h"
#include "regs.h"

namespace debugger {
namespace z80 {

struct RegsZ80 final : Regs {
    RegsZ80(const char *name, PinsZ80Base &pins) : _name(name), _pins(pins) {}
    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint16_t addr) { _pc = addr; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint8_t n) const override;
    void setRegister(uint8_t reg, uint32_t value) override;

    uint8_t read_io(uint8_t addr) const;
    void write_io(uint8_t addr, uint8_t data) const;

private:
    const char *_name;
    PinsZ80Base &_pins;
    uint16_t _pc;
    uint16_t _sp;
    uint16_t _ix;
    uint16_t _iy;
    struct reg {
        uint8_t a;
        uint8_t f;
        uint8_t b;
        uint8_t c;
        uint8_t d;
        uint8_t e;
        uint8_t h;
        uint8_t l;
        uint16_t bc() const { return uint16(b, c); }
        uint16_t de() const { return uint16(d, e); }
        uint16_t hl() const { return uint16(h, l); }
        void setbc(uint16_t v) {
            b = hi(v);
            c = lo(v);
        }
        void setde(uint16_t v) {
            d = hi(v);
            e = lo(v);
        }
        void sethl(uint16_t v) {
            h = hi(v);
            l = lo(v);
        }
    } _main, _alt;
    uint8_t _i;
    uint8_t _r;

    void exchangeRegs() const;
    void saveRegs(reg &regs) const;
    void restoreRegs(const reg &regs) const;
};

}  // namespace z80
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

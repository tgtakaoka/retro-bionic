#ifndef __REGS_MC68HC12_H__
#define __REGS_MC68HC12_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace mc68hc12 {

struct PinsMc68hc12;
struct Mc68hc12Init;

struct RegsMc68hc12 final : Regs {
    RegsMc68hc12(PinsMc68hc12 *pins, Mc68hc12Init &init);

    const char *cpu() const override;

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;
    // An exception stacked |frame|, the 9 bytes from SP up.
    void capture(uint16_t sp, const uint8_t *frame, bool breakTrap);
    // The frame stays stacked and the CPU goes on at |pc|, a handler.
    void vectored(uint16_t pc);

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override { _pc = addr; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    uint8_t internal_read(uint16_t addr) const;
    void internal_write(uint16_t addr, uint8_t data) const;

    static constexpr uint8_t FRAME = 9;  // CCR B A XH XL YH YL PCH PCL

private:
    PinsMc68hc12 *const _pins;
    Mc68hc12Init &_init;

    uint16_t _pc;
    uint16_t _sp;
    uint16_t _x;
    uint16_t _y;
    uint8_t _a;
    uint8_t _b;
    uint8_t _cc;

    mutable CharBuffer _buffer;

    void _d(uint16_t d) {
        _a = hi(d);
        _b = lo(d);
    }
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

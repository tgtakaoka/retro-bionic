#ifndef __REGS_I8080_H__
#define __REGS_I8080_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace i8080 {

struct PinsI8080Base;

struct RegsI8080 : Regs {
    RegsI8080(PinsI8080Base *pins);

    const char *cpu() const override;

    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    void saveContext(uint16_t pc, uint8_t inte);

protected:
    PinsI8080Base *const _pins;

    uint8_t _b;
    uint8_t _c;
    uint8_t _d;
    uint8_t _e;
    uint8_t _h;
    uint8_t _l;
    uint8_t _a;
    uint8_t _psw;
    uint16_t _pc;
    uint16_t _sp;
    uint8_t _ie;

    void _bc(uint16_t bc) {
        _c = lo(bc);
        _b = hi(bc);
    }

    void _de(uint16_t de) {
        _e = lo(de);
        _d = hi(de);
    }

    void _hl(uint16_t hl) {
        _l = lo(hl);
        _h = hi(hl);
    }

    virtual bool ie() const;

    mutable CharBuffer _buffer;
};

}  // namespace i8080
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

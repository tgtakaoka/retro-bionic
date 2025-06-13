#ifndef __REGS_Z8_H__
#define __REGS_Z8_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace z8 {

struct PinsZ8;

struct RegsZ8 : Regs {
    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    const RegList *listRegisters(uint_fast8_t reg) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

protected:
    RegsZ8(PinsZ8 *pins);
    PinsZ8 *_pins;

    uint16_t _pc;
    uint16_t _sp;
    uint8_t _flags;
    virtual void save_sfrs() = 0;
    virtual void restore_sfrs() = 0;

    uint8_t _r[16];
    void save_all_r();
    virtual uint_fast8_t save_r(uint_fast8_t num) const = 0;
    virtual void restore_r(uint_fast8_t num, uint8_t val) const = 0;

    virtual void set_flags(uint_fast8_t flags) = 0;
    virtual void set_rp(uint_fast8_t rp) = 0;
    virtual void set_sp(uint16_t sp) = 0;
    virtual void set_r(uint_fast8_t num, uint_fast8_t val) = 0;
    void set_rr(uint_fast8_t num, uint16_t val) {
        set_r(num, hi(val));
        set_r(num + 1, lo(val));
    }

    static const char *const REGS8[18];
    static const char *const REGS16[10];

    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;
};

}  // namespace z8
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

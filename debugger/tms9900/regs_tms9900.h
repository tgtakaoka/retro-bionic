#ifndef __REGS_TMS9900_H__
#define __REGS_TMS9900_H__

#include "char_buffer.h"
#include "mems.h"
#include "regs.h"

namespace debugger {
namespace tms9900 {

struct PinsTms9900Base;

struct RegsTms9900 : Regs {
    RegsTms9900(PinsTms9900Base *pins, Mems *mems);

    const char *cpu() const override;

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    virtual void breakPoint();
    virtual uint16_t read_reg(uint8_t i) const;
    virtual void write_reg(uint8_t i, uint16_t data) const;

protected:
    PinsTms9900Base *const _pins;
    Mems *const _mems;

    uint16_t _wp;
    uint16_t _pc;
    uint16_t _st;

    template <typename PINS>
    PINS *pins() const {
        return static_cast<PINS *>(_pins);
    }

    template <typename T, uint_fast8_t SIZE>
    inline auto length(const T (&array)[SIZE]) const {
        return SIZE;
    }
    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;

    static const uint16_t ZERO;
};

}  // namespace tms9900
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

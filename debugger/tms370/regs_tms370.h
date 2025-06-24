#ifndef __REGS_TMS370_H__
#define __REGS_TMS370_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace tms370 {

struct PinsTms370;

struct RegsTms370 : Regs {
    const char *cpu() const override;

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override { _pc = addr; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    uint_fast8_t read_internal(uint16_t add);
    void write_internal(uint16_t add, uint8_t data);
    uint_fast8_t read_peripheral(uint8_t add);
    void write_peripheral(uint8_t add, uint8_t data);

protected:
    RegsTms370(PinsTms370 *pins);

    PinsTms370 *const _pins;

    uint8_t _a;
    uint8_t _b;
    uint8_t _sp;
    uint8_t _st;
    uint16_t _pc;

    static constexpr uint16_t A = 0x0000;
    static constexpr uint16_t B = 0x0001;

    mutable CharBuffer _buffer;
};

}  // namespace tms370
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

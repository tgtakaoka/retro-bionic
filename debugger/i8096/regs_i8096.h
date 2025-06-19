#ifndef __REGS_I8096_H__
#define __REGS_I8096_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace i8096 {

struct PinsI8096;

struct RegsI8096 final : Regs {
    RegsI8096(PinsI8096 *pins);

    const char *cpu() const override;

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    uint16_t read_data(uint_fast8_t addr) const;
    void write_data(uint_fast8_t addr, uint16_t data) const;
    uint16_t read_data16(uint_fast8_t addr) const;
    void write_data16(uint_fast8_t addr, uint16_t data) const;

private:
    PinsI8096 *const _pins;

    uint16_t _pc;
    uint16_t _sp;
    uint16_t _psw;

    mutable CharBuffer _buffer1;

    template <typename T, uint_fast8_t SIZE>
    inline auto length(const T (&array)[SIZE]) const {
        return SIZE;
    }

    static constexpr auto ADDR_SP = 0x18;
    void nops(uint_fast8_t len = 4) const;
};

}  // namespace i8096
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

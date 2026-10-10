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
    // A trap pushed |pc| below |sp|.
    void captureContext(uint16_t sp, uint16_t pc, bool breakTrap);

    uint32_t nextIp() const override { return _pc; }
    uint16_t sp() const { return _sp; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    uint16_t read_data(uint16_t addr) const;
    void write_data(uint16_t addr, uint16_t data) const;
    uint16_t read_data16(uint8_t addr) const;
    void write_data16(uint8_t addr, uint16_t data) const;

private:
    PinsI8096 *const _pins;

    uint16_t _pc;
    uint16_t _sp;
    uint16_t _psw;
    // The 80C196's, PUSHA pushes them too.
    uint8_t _int_mask1;
    uint8_t _wsr;

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;

    uint16_t read_upper16(uint16_t addr) const;
    void write_upper16(uint16_t addr, uint16_t data) const;

    template <typename T, uint_fast8_t SIZE>
    inline auto length(const T (&array)[SIZE]) const {
        return SIZE;
    }

    static constexpr auto ADDR_INT_PENDING = 0x09;
    static constexpr auto ADDR_INT_PENDING1 = 0x12;
    static constexpr auto ADDR_WSR = 0x14;
    static constexpr auto ADDR_SP = 0x18;
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

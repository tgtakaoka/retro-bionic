#ifndef __REGS_MN1613_H__
#define __REGS_MN1613_H__

#include "char_buffer.h"
#include "mems.h"
#include "regs.h"

namespace debugger {
namespace mn1613 {

struct PinsMn1613;
struct MemsMn1613;

struct RegsMn1613 final : Regs {
    RegsMn1613(PinsMn1613 *pins, MemsMn1613 *mems);

    const char *cpu() const override;
    const char *cpuName() const override;

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override;
    void setIp(uint32_t pc) override;
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    uint32_t addIp(int_fast8_t n) const;
    void checkCpuType();

private:
    PinsMn1613 *const _pins;
    MemsMn1613 *const _mems;
    bool _mn1613A;

    uint16_t _reg[8];
    static constexpr auto R0 = 0;
    static constexpr auto SP = 5;
    static constexpr auto STR = 6;
    static constexpr auto IC = 7;
    uint8_t _seg[4];
    static constexpr auto CSBR = 0;
    static constexpr auto SSBR = 1;
    static constexpr auto TSR0 = 2;
    static constexpr auto TSR1 = 3;
    static uint32_t addr(uint_fast8_t seg, uint16_t off);
    static constexpr auto NPP = 2;
    uint16_t _npp;
    static constexpr auto IISR = 6;
    uint8_t _iisr;

    uint16_t getReg(uint_fast8_t rs) const;
    void setReg(uint_fast8_t rd, uint16_t data) const;
    uint16_t captureReg(uint16_t inst) const;
    void injectReg(uint16_t inst, uint16_t data) const;

    template <typename T, uint_fast8_t SIZE>
    inline auto length(const T (&array)[SIZE]) const {
        return SIZE;
    }

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
};

}  // namespace mn1613
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

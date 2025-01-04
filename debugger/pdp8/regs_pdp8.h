#ifndef __REGS_PDP8_H__
#define __REGS_PDP8_H__

#include "regs.h"

namespace debugger {
namespace pdp8 {

struct PinsPdp8;

struct RegsPdp8 : Regs {
    const char *cpuName() const override;

    void reset() override;
    virtual void breakPoint() = 0;

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t pc) override { _pc = pc; }
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    uint16_t readSwitch() const { return _sw; }
    void writeSwitch(uint16_t value) { _sw = value; }

protected:
    RegsPdp8(PinsPdp8 *pins);

    PinsPdp8 *const _pins;
    template <typename PINS>
    PINS pins() const {
        return static_cast<PINS *>(_pins);
    }

    template <typename T, uint_fast8_t SIZE>
    inline auto length(const T (&array)[SIZE]) {
        return SIZE;
    }

    static constexpr auto REG_AC = 1;
    static constexpr auto REG_MQ = 2;
    static constexpr auto REG_PC = 3;
    static constexpr auto REG_SW = 4;
    static constexpr auto REG_LINK = 7;
    static constexpr auto REG_IEFF = 8;

    uint16_t _ac;
    uint16_t _mq;
    uint16_t _pc;
    uint16_t _sw = 0;
    uint16_t _flags;
    static constexpr auto link = 04000;
    static constexpr auto ieff = 00200;
    uint_fast8_t _link() const { return (_flags & link) >> 11; }
    uint_fast8_t _ieff() const { return (_flags & ieff) >> 7; }
    void set_link(uint_fast8_t value) {
        _flags &= ~link;
        _flags |= value << 11;
    }
    void set_ieff(uint_fast8_t value) {
        _flags &= ~ieff;
        _flags |= value << 7;
    }
};

}  // namespace pdp8
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

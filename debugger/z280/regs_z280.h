#ifndef __REGS_Z280_H__
#define __REGS_Z280_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace z280 {

struct PinsZ280;

struct RegsZ280 final : Regs {
    RegsZ280(PinsZ280 *pins);

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

private:
    PinsZ280 *const _pins;

    uint16_t _pc;
    uint16_t _st;
    uint32_t _acc;
    uint16_t _ar[2];
    uint32_t _p;
    uint16_t _t;

    template <typename T, uint_fast8_t SIZE>
    inline auto length(const T (&array)[SIZE]) const {
        return SIZE;
    }

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;

    // working data memory (on page 1 but maybe page 0)
    static constexpr uint16_t DMA = 0x8F;
    static constexpr uint16_t DIR_DMA = DMA & 0x7F;
    // indirect AR (*AR)
    static constexpr uint16_t INDIR_AR = 0x88;

    uint32_t saveAcc() const;
    uint16_t saveAr(uint_fast8_t n) const;
    void loadReg(uint_fast8_t dma, uint16_t val, uint16_t inst) const;
};

}  // namespace z280
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __REGS_F3850_H__
#define __REGS_F3850_H__

#include "char_buffer.h"
#include "devs.h"
#include "mems.h"
#include "regs.h"
#include "signals_f3850.h"

namespace debugger {
namespace f3850 {

struct PinsF3850;

struct RegsF3850 final : Regs {
    RegsF3850(PinsF3850 *pins, Devs *devs);

    const char *cpu() const override;
    const char *cpuName() const override { return cpu(); }

    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc0; }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    bool romc_read(Signals *signals);
    bool romc_write(Signals *signals);
    uint8_t read_reg(uint8_t addr);
    void write_reg(uint8_t addr, uint8_t val);
    uint8_t read_io(uint8_t addr);
    void write_io(uint8_t addr, uint8_t val);

private:
    PinsF3850 *const _pins;
    Devs *const _devs;
    friend PinsF3850;
    Mems *_mems;

    uint16_t _pc0;
    uint16_t _pc1;
    uint16_t _dc0;
    uint16_t _dc1;
    uint8_t _isar;
    uint8_t _a;
    uint8_t _w;
    uint8_t _r[16];
    uint8_t _ioaddr;

    static constexpr uint8_t J = 9;
    static constexpr uint8_t HU = 10;
    static constexpr uint8_t HL = 11;
    static constexpr uint8_t KU = 12;
    static constexpr uint8_t KL = 13;
    static constexpr uint8_t QU = 14;
    static constexpr uint8_t QL = 15;

    uint8_t isl() const { return _isar & 7; }
    uint8_t isu() const { return (_isar >> 3) & 7; }
    void set_isl(uint8_t val);
    void set_isu(uint8_t val);
    void update_r(uint8_t num, uint8_t val);

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;
};

}  // namespace f3850
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __REGS_I8086_H__
#define __REGS_I8086_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace i8086 {

struct PinsI8086;

struct RegsI8086 final : Regs {
    RegsI8086(PinsI8086 *pins);

    const char *cpu() const override;
    // The chip reset found.
    const char *cpuName() const override;

    void print() const override;
    void save() override;
    void restore() override { restore(false); }
    // With |step| the restored FLAGS set TF: the CPU traps after one
    // instruction.
    void restore(bool step);
    // Whether the parked CPU is a V20 or V30: they take AAD's base as 10,
    // whatever its immediate says. AX and FLAGS are lost.
    bool probeVSeries();

    uint32_t nextIp() const override;
    void setIp(uint32_t addr) override;
    // Parked in the program's fetch at |cs|:|ip|.
    void parkAt(uint16_t cs, uint16_t ip);
    // Parked at PARK after the debugger's trap |vector|; its frame held
    // the program's CS, IP and FLAGS.
    void parkInTrap(uint16_t cs, uint16_t ip, uint16_t flags, uint8_t vector);

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsI8086 *const _pins;
    // AX, CX, DX, BX, SP, BP, SI, DI: the order the encodings number them.
    uint16_t _r[8];
    // ES, CS, SS, DS: the same.
    uint16_t _s[4];
    uint16_t _ip;
    uint16_t _flags;
    // Where the CPU is parked.
    uint16_t _parkCs;
    uint16_t _parkIp;
    bool _frame = false;  // a trap frame lies below SP

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
};

}  // namespace i8086
}  // namespace debugger
#endif /* __REGS_I8086_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

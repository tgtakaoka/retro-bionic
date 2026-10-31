#ifndef __REGS_NS32016_H__
#define __REGS_NS32016_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace ns32016 {

struct PinsNs32016;
struct MemsNs32016;

struct RegsNs32016 final : Regs {
    RegsNs32016(PinsNs32016 *pins, MemsNs32016 *mems);

    // libasm's name for the instruction set.
    const char *cpu() const override { return "NS32032"; }
    const char *cpuName() const override;

    void print() const override;
    void reset() override;
    void save() override;
    void restore() override { restore(false); }
    // With |trace|, the program's first instruction ends in a trace trap.
    void restore(bool trace);

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override { _pc = addr; }
    uint32_t intbase() const { return _intbase; }
    // Parked in the debugger's trap handler, the program's PC, MOD and PSR
    // in the frame it pushed.
    void parkInTrap(uint32_t pc, uint16_t mod, uint16_t psr);

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsNs32016 *const _pins;
    MemsNs32016 *const _mems;

    uint32_t _r[8];
    uint32_t _pc;
    uint16_t _psr;
    uint16_t _mod;
    uint32_t _sb;
    uint32_t _fp;
    uint32_t _sp0;
    uint32_t _sp1;
    uint32_t _intbase;
    uint32_t _f[8];
    uint32_t _fsr;
    // Where the CPU waits: the trap handler, or the program's PC.
    uint32_t _parkedAt;
    // Parked in a trap: the frame lies below SP0, and SB is the module's.
    bool _frame = false;
    // Restored for a step: the trap frame's T is the debugger's.
    bool _tracing = false;

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;
};

}  // namespace ns32016
}  // namespace debugger
#endif /* __REGS_NS32016_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

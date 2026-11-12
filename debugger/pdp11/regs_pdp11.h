#ifndef __REGS_PDP11_H__
#define __REGS_PDP11_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace pdp11 {

struct PinsPdp11;

struct RegsPdp11 final : Regs {
    // |fpu|: the FP11's FPS and AC0-AC5 are saved and restored too.
    RegsPdp11(PinsPdp11 *pins, bool fpu = false);

    const char *cpu() const override { return cpuName(); }

    void print() const override;
    void printAll() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override { park(addr, addr); }
    // Parked in the program's fetch of |pc|, |addr| on the bus.
    void park(uint16_t pc, uint16_t addr) {
        _pc = pc;
        _parkedAt = addr;
        _frame = false;
    }
    // Parked after a trap the debugger took; the program's PC and PS are
    // in the frame it pushed.
    void parkInTrap(uint16_t pc, uint16_t ps, uint16_t addr) {
        _pc = pc;
        _ps = ps;
        _parkedAt = addr;
        _frame = true;
    }
    uint16_t parkedAt() const { return _parkedAt; }
    // The next restore() sets the T bit, to trap after one instruction.
    void setTrace(bool trace) { _trace = trace; }
    // A kernel stack below 400, where the DCJ11's push of a trap frame
    // takes a yellow stack trap (DCJ11 User's Guide, 1.8).
    bool kernelStackLow() const { return (_ps & 0140000) == 0 && _sp < 0400; }

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsPdp11 *const _pins;
    const bool _fpu;

    uint16_t _r[6];
    uint16_t _sp;
    uint16_t _pc;
    uint16_t _ps;
    uint16_t _parkedAt;
    bool _frame = false;  // a trap frame lies below SP
    bool _trace = false;
    uint16_t _fps;
    // Double precision, the most significant word first.
    uint16_t _ac[6][4];

    void saveFpu();
    void restoreFpu();

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __REGS_PDP11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

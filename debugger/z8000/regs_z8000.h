#ifndef __REGS_Z8000_H__
#define __REGS_Z8000_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace z8000 {

struct PinsZ8000;

struct RegsZ8000 final : Regs {
    RegsZ8000(PinsZ8000 *pins);

    const char *cpu() const override { return cpuName(); }

    void print() const override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override { park(addr, addr); }
    // Parked in the program's fetch of |pc|, |addr| on the bus.
    void park(uint32_t pc, uint32_t addr) {
        _pc = pc;
        _parkedAt = addr;
        _frame = false;
    }
    // Parked after a debugger trap; the program's PC and FCW are in its
    // frame.
    void parkInTrap(uint32_t pc, uint16_t fcw, uint32_t addr) {
        _pc = pc;
        _fcw = fcw;
        _parkedAt = addr;
        _frame = true;
    }
    void setFcw(uint16_t fcw) { _fcw = fcw; }
    uint16_t fcw() const { return _fcw; }
    // Segment 0 for what a nonsegmented program can't set.
    void clearSegments() {
        _nsp &= UINT16_MAX;
        _nspseg = 0;
        _psap &= UINT16_MAX;
        _r[14] = 0;
    }
    uint32_t parkedAt() const { return _parkedAt; }

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsZ8000 *const _pins;
    void saveSegmented(uint8_t *buffer, uint32_t &org);
    void restoreSegmented();

    uint16_t _r[16];  // R15 (RR14 on the Z8001) is the system stack
    uint32_t _pc;
    uint16_t _fcw;
    uint32_t _nsp;
    uint16_t _nspseg = 0;  // a nonsegmented Z8001 program's R14
    // In normal mode the program's R15 (RR14 on the Z8001) is NSP.
    bool normalMode() const;
    bool programR14() const;
    uint32_t ssp() const;
    void setR14(uint16_t v);
    void setR15(uint16_t v);
    uint32_t _psap;
    uint32_t _parkedAt;
    bool _frame = false;  // a trap frame lies below R15

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;
};

}  // namespace z8000
}  // namespace debugger
#endif /* __REGS_Z8000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

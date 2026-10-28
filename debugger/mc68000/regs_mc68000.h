#ifndef __REGS_MC68000_H__
#define __REGS_MC68000_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace mc68000 {

struct PinsMc68000;
struct MemsMc68000;

struct RegsMc68000 final : Regs {
    RegsMc68000(PinsMc68000 *pins, MemsMc68000 *mems);

    const char *cpu() const override {
        return _mc68010 ? "MC68010" : "MC68000";
    }
    // An MC68010 stacks a format/vector word below SR and PC: a frame of
    // eight bytes, not six.
    void setMc68010(bool mc68010) { _mc68010 = mc68010; }
    uint_fast8_t frameBytes() const { return _mc68010 ? 8 : 6; }

    void print() const override;
    // The state the program's reset vector sets, applied on resume.
    void reset() override;
    void save() override;
    void restore() override { restore(false); }
    // With |trace|, T is set in the restored SR: the CPU takes the trace
    // exception after one instruction.
    void restore(bool trace);

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override { parkAtFetch(addr); }
    // Parked at the fetch of |addr|, outside a vector.
    void parkAtFetch(uint32_t addr) {
        _pc = addr;
        _parkedAddr = addr;
        _vector = false;
    }
    // The frame an exception stacked (M68000UM Figure 6-5): SR, then PC,
    // then on the MC68010 the format/vector word.
    struct Frame {
        uint32_t pc;   // the program's PC to resume at
        uint16_t sr;
        uint32_t ssp;  // SSP before the frame was stacked
    };
    // Stopped at the read of an exception vector, with the program's PC
    // and SR stacked.
    void parkInVector(uint32_t vecAddr, const Frame &frame) {
        _vector = true;
        _vecAddr = vecAddr;
        _pc = frame.pc;
        _sr = frame.sr;
        _ssp = frame.ssp;
    }
    // Where the CPU is parked, as the bus showed it, which is where
    // injection resumes.
    uint32_t parkedAt() const { return _vector ? _vecAddr : _parkedAddr; }

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsMc68000 *const _pins;
    MemsMc68000 *const _mems;
    bool _mc68010 = false;

    uint32_t _pc;
    uint32_t _parkedAddr;
    bool _vector = false;
    uint32_t _vecAddr;
    uint16_t _sr;
    uint32_t _ssp;
    uint32_t _usp;
    uint32_t _d[8];
    uint32_t _a[7];

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __REGS_MC68000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

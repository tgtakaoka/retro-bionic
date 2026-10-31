#ifndef __REGS_H8500_H__
#define __REGS_H8500_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace h8500 {

struct PinsH8500;
struct MemsH8500;

struct RegsH8500 final : Regs {
    RegsH8500(PinsH8500 *pins, MemsH8500 *mems);

    const char *cpu() const override { return "H8/500"; }
    const char *cpuName() const override;

    void print() const override;
    // The state the program's reset vector sets, applied on resume.
    void reset() override;
    void save() override;
    void restore() override { restore(false); }
    // With |trace| the program resumes with T set: one instruction, then
    // the trace exception.
    void restore(bool trace);

    // CP:PC.
    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override {
        _pc = addr;
        _parkedAddr = addr;
        _vector = false;
    }

    // The frame an exception stacked, as memory holds it.
    struct Frame {
        uint32_t pc;  // CP:PC to resume at
        uint16_t sr;
        uint32_t sp;  // TP:SP before the frame was stacked
    };
    // Stopped at the read of an exception vector, with the program's CP,
    // PC and SR stacked.
    void parkInVector(uint32_t vecAddr, const Frame &frame) {
        _vector = true;
        _vecAddr = vecAddr & ~UINT32_C(3);
        _pc = frame.pc;
        _sr = frame.sr;
        _r[7] = frame.sp;
        _tp = frame.sp >> 16;
    }
    // Where the CPU is parked, as the bus showed it, which is where
    // injection resumes.
    uint32_t parkedAt() const { return _vector ? _vecAddr : _parkedAddr; }

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsH8500 *const _pins;
    MemsH8500 *const _mems;

    uint32_t _parkedAddr;
    bool _vector = false;
    uint32_t _vecAddr;
    uint32_t _pc;
    uint16_t _sr;  // CCR is its low byte
    // R0-R7; R7 is SP, R6 FP.
    uint16_t _r[8];
    uint8_t _br;
    uint8_t _ep;
    uint8_t _dp;
    uint8_t _tp;

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
};

}  // namespace h8500
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

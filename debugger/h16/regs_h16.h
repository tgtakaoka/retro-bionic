#ifndef __REGS_H16_H__
#define __REGS_H16_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace h16 {

struct PinsH16;
struct MemsH16;

struct RegsH16 final : Regs {
    RegsH16(PinsH16 *pins, MemsH16 *mems);

    const char *cpu() const override { return "H16"; }
    const char *cpuName() const override { return "HD641016"; }

    void print() const override;
    // The state the program's reset vector sets, applied on resume.
    void reset() override;
    void save() override;
    void restore() override { restore(false); }
    // With |trace| the program resumes with T set: one instruction, then
    // the trace exception.
    void restore(bool trace);

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override {
        _pc = addr;
        _parkedAddr = addr;
        _vector = false;
    }
    uint32_t ebr() const { return _ebr; }

    // The frame an exception stacked, as memory holds it.
    struct Frame {
        uint32_t pc;  // the program's PC to resume at
        uint16_t sr;
        uint32_t sp;  // SSP before the frame was stacked
    };
    // Stopped at the read of an exception vector, with the program's PC
    // and SR stacked.
    void parkInVector(uint32_t vecAddr, const Frame &frame) {
        _vector = true;
        _vecAddr = vecAddr;
        _pc = frame.pc;
        _sr = frame.sr;
        _ssp = frame.sp;
    }
    // Where the CPU is parked, as the bus showed it, which is where
    // injection resumes.
    uint32_t parkedAt() const { return _vector ? _vecAddr : _parkedAddr; }

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsH16 *const _pins;
    MemsH16 *const _mems;

    uint32_t _parkedAddr;
    bool _vector = false;
    uint32_t _vecAddr;
    uint32_t _pc;
    uint16_t _sr;  // CCR is its low byte
    uint32_t _ssp;
    uint32_t _usp;
    // R0-R14 of the current bank; R15 is SSP or USP.
    uint32_t _r[15];
    uint32_t _ebr;
    uint32_t _rbr;
    uint32_t _ibr;
    uint32_t _bsp;
    uint32_t _cbnr;
    uint8_t _bmr;
    uint8_t _gbnr;
    uint8_t _vbnr;

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;
    mutable CharBuffer _buffer4;
    mutable CharBuffer _buffer5;
};

}  // namespace h16
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

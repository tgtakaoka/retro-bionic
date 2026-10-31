#ifndef __REGS_H8300_H__
#define __REGS_H8300_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace h8300 {

struct PinsH8300;
struct MemsH8300;

struct RegsH8300 final : Regs {
    RegsH8300(PinsH8300 *pins, MemsH8300 *mems);

    const char *cpu() const override { return "H8/300"; }
    const char *cpuName() const override { return "H8/323"; }

    void print() const override;
    // The state the program's reset vector sets, applied on resume.
    void reset() override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override {
        _pc = addr;
        _parkedAddr = addr;
        _vector = false;
    }

    // What an exception stacked: the PC and CCR, and SP before it.
    struct Frame {
        uint16_t pc;
        uint8_t ccr;
        uint16_t sp;
    };
    // Stopped at the read of an exception vector, with the program's PC
    // and CCR stacked.
    void parkInVector(uint16_t vecAddr, const Frame &frame) {
        _vector = true;
        _vecAddr = vecAddr & ~1;
        _pc = frame.pc;
        _ccr = frame.ccr;
        _r[7] = frame.sp;
    }
    // Where the CPU is parked, as the bus showed it, which is where
    // injection resumes.
    uint16_t parkedAt() const { return _vector ? _vecAddr : _parkedAddr; }

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsH8300 *const _pins;
    MemsH8300 *const _mems;

    uint16_t _parkedAddr;
    bool _vector = false;
    uint16_t _vecAddr;
    uint16_t _pc;
    uint8_t _ccr;
    // R0-R7; R7 is SP.
    uint16_t _r[8];

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
};

}  // namespace h8300
}  // namespace debugger
#endif /* __REGS_H8300_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __REGS_MC68HC16_H__
#define __REGS_MC68HC16_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace mc68hc16 {

struct PinsMc68hc16;
struct MemsMc68hc16;

struct RegsMc68hc16 final : Regs {
    RegsMc68hc16(PinsMc68hc16 *pins, MemsMc68hc16 *mems);

    const char *cpu() const override { return "MC68HC16"; }

    void print() const override;
    // The state the program's reset vector sets, applied on resume.
    void reset() override;
    void save() override;
    void restore() override;

    uint32_t nextIp() const override { return _pc; }
    void setIp(uint32_t addr) override { park(addr); }
    // Parked at the fetch of |addr|, outside a vector.
    void park(uint32_t addr) {
        _pc = addr;
        _parkedAddr = addr;
        _vector = NONE;
    }
    // How the CPU was stopped: at the read of an exception vector, with
    // the program's PC and CCR stacked.
    enum Vector : uint8_t {
        NONE,  // parked at a fetch outside a vector
        IRQ7,  // #IRQ7, the debugger's halt and step
        SWI,   // a breakpoint or the halt convention
    };
    // The stacked frame as memory holds it.
    struct Frame {
        uint32_t pc;   // the program's PK:PC to resume at
        uint16_t ccr;  // with PK in its low nibble
        uint32_t sp;   // SK:SP before the frame was stacked
    };
    void parkInVector(Vector how, uint32_t vecAddr, const Frame &frame) {
        _vector = how;
        _vecAddr = vecAddr;
        _pc = frame.pc;
        _ccr = (frame.ccr & ~0xF) | ((frame.pc >> 16) & 0xF);
        _sp = frame.sp;
    }
    // Where the CPU is parked, as the bus showed it, which is where
    // injection resumes.
    uint32_t parkedAt() const {
        return _vector != NONE ? _vecAddr : _parkedAddr;
    }
    // Changes with any register the program can change.
    uint32_t fingerprint() const;

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsMc68hc16 *const _pins;
    MemsMc68hc16 *const _mems;

    uint32_t _pc;  // PK:PC
    uint32_t _parkedAddr;
    Vector _vector = NONE;
    uint32_t _vecAddr;
    uint32_t _sp;   // SK:SP
    uint16_t _ccr;  // PK kept with _pc
    uint16_t _d;
    uint16_t _e;
    uint16_t _ix;
    uint16_t _iy;
    uint16_t _iz;
    uint16_t _k;  // XK:YK:ZK:EK
    // The MAC unit as PSHMAC stacks it, a word per entry from the SP at
    // the push down (CPU16RM 11.7.5.1): H, I, AM[15:0], AM[31:16],
    // SL|AM[35:32], XMSK:YMSK.
    uint16_t _mac[6];

    uint8_t xk() const { return _k >> 12; }
    uint8_t yk() const { return (_k >> 8) & 0xF; }
    uint8_t zk() const { return (_k >> 4) & 0xF; }
    uint8_t ek() const { return _k & 0xF; }
    void setK(uint_fast8_t shift, uint32_t v) {
        _k = (_k & ~(0xF << shift)) | ((v & 0xF) << shift);
    }

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;
};

}  // namespace mc68hc16
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

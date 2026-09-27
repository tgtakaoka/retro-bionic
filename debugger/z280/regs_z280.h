#ifndef __REGS_Z280_H__
#define __REGS_Z280_H__

#include "char_buffer.h"
#include "regs.h"

namespace debugger {
namespace z280 {

struct PinsZ280;
struct MemsZ280;

struct RegsZ280 final : Regs {
    RegsZ280(PinsZ280 *pins, MemsZ280 *mems);

    const char *cpu() const override { return cpuName(); }

    void print() const override;
    void save() override;
    void restore() override;

    // The MMU's page is 4K: the low 12 bits of an address are the
    // offset, the rest its page (logical) or page frame (physical).
    static constexpr uint16_t pageOf(uint16_t logical) { return logical >> 12; }
    static constexpr uint16_t pageFrameOf(uint32_t physical) {
        return physical >> 12;
    }
    static constexpr uint32_t physicalOf(uint16_t pageFrame, uint16_t logical) {
        return (static_cast<uint32_t>(pageFrame) << 12) | (logical & 0xFFF);
    }

    // The bus showed where the fetch of _pc is physically; the page
    // frame is kept with the PC, and resuming puts the two back
    // together -- the MMU emulated for the one page the debugger has
    // seen.
    uint32_t nextIp() const override { return physicalPc(); }
    void setIp(uint32_t addr) override { park(addr, addr); }
    // Park at logical |pc|, whose fetch the bus showed at |addr|.
    void park(uint16_t pc, uint32_t addr) {
        _pc = pc;
        _pageFrame = pageFrameOf(addr);
        _parkedAddr = addr;
        _vector = NONE;
    }
    // How the CPU was stopped: at the fetch of a vector, with the
    // program's status on the system stack.
    enum Vector : uint8_t {
        NONE,  // parked at the program's own fetch (after reset)
        RST,   // RST 38H: the return address pushed
        NMI,   // NMI, interrupt modes 0-2: PC pushed
        NMI3,  // NMI, interrupt mode 3: PC and MSR pushed
    };
    // Park at the vector fetch |vecAddr|: the program stopped at logical
    // |pc| (fetching in the page |addr| is in), its status is at |frame|
    // on the system stack. Every injected sequence runs from the vector,
    // in system mode; restore() returns the way the vector was taken.
    void parkInVector(Vector how, uint16_t pc, uint32_t addr, uint32_t frame,
            uint32_t vecAddr, uint16_t msr = 0) {
        _pc = pc;
        _pageFrame = pageFrameOf(addr);
        _vector = how;
        _frameAddr = frame;
        _vecAddr = vecAddr;
        _msr = msr;
    }
    // A step runs one instruction: leave the cache off for it.
    void cacheForStep(bool hold) { _holdCache = hold; }
    // A new PC keeps the context: on the parked page it keeps the
    // frame, off it the frame is unknown and identity is assumed.
    void setPc(uint16_t pc) {
        if (pageOf(pc) != pageOf(_pc))
            _pageFrame = pageOf(pc);
        _pc = pc;
    }
    // Where the program's PC is physically.
    uint32_t physicalPc() const { return physicalOf(_pageFrame, _pc); }
    // Where the CPU is parked, as the bus showed it, which is where
    // injection resumes: with the cache on that is the even word
    // holding the PC, not the PC itself.
    uint32_t parkedAt() const {
        return _vector != NONE ? _vecAddr : _parkedAddr;
    }
    // A logical address on the parked page maps through its frame. Off
    // that page the frame is unknown, and identity is what a disabled
    // MMU does (section 7.2). At a vector the CPU is in system mode,
    // whose map is assumed to be identity.
    uint32_t physical(uint16_t logical) const {
        if (_vector == NONE && pageOf(logical) == pageOf(_pc))
            return physicalOf(_pageFrame, logical);
        return logical;
    }
    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

private:
    PinsZ280 *const _pins;
    MemsZ280 *const _mems;

    uint16_t _ix;
    uint16_t _iy;
    uint16_t _pc;
    uint16_t _pageFrame;  // of the fetch at _pc, as the bus showed it
    uint32_t _parkedAddr;  // the fetch the CPU is parked in, outside a vector
    // Parked at a vector (see parkInVector()).
    Vector _vector = NONE;
    uint16_t _msr;        // the program's MSR: read, or as a mode 3 NMI pushed it
    uint32_t _frameAddr;  // the program's status on the system stack
    uint32_t _vecAddr;    // the vector fetch the CPU is parked in
    bool _holdCache = false;  // next restore(): keep the cache off
    bool _cacheHeld = false;  // the cache is off by the debugger's doing
    // The debugger runs the CPU in system mode, so _sp is the System
    // Stack Pointer; the User Stack Pointer is a separate register
    // reachable only through LDCTL.
    uint16_t _sp;
    uint16_t _usp;
    uint8_t _iop;  // I/O Page register
    uint8_t _cache = 0x60;  // Cache Control: the program's; 60H caches nothing
    struct reg {
        uint8_t a;
        uint8_t f;
        uint8_t b;
        uint8_t c;
        uint8_t d;
        uint8_t e;
        uint8_t h;
        uint8_t l;
        uint16_t bc() const { return uint16(b, c); }
        uint16_t de() const { return uint16(d, e); }
        uint16_t hl() const { return uint16(h, l); }
        void setbc(uint16_t v) {
            b = hi(v);
            c = lo(v);
        }
        void setde(uint16_t v) {
            d = hi(v);
            e = lo(v);
        }
        void sethl(uint16_t v) {
            h = hi(v);
            l = lo(v);
        }
    } _main, _alt;
    uint8_t _i;
    uint8_t _r;

    void exchangeRegs(uint32_t &org) const;
    void saveRegs(reg &regs, uint32_t &org) const;
    void restoreRegs(const reg &regs, uint32_t &org) const;

    mutable CharBuffer _buffer1;
    mutable CharBuffer _buffer2;
    mutable CharBuffer _buffer3;
};

}  // namespace z280
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

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
        _inNmi = false;
    }
    // Park inside a mode 3 NMI service: the program stopped at logical
    // |pc| (fetching in the page |addr| is in), its PC and MSR are on
    // the system stack with the MSR at |msrAddr|, and the CPU is in the
    // fetch of the handler at |handlerAddr|, in system mode. Every
    // injected sequence runs there; restore() returns with RETIL.
    void parkInNmi(uint16_t pc, uint32_t addr, uint16_t msr,
            uint32_t msrAddr, uint32_t handlerAddr) {
        _pc = pc;
        _pageFrame = pageFrameOf(addr);
        _msr = msr;
        _sspAddr = msrAddr;
        _handler = handlerAddr;
        _inNmi = true;
    }
    // A new PC keeps the context: on the parked page it keeps the
    // frame, off it the frame is unknown and identity is assumed.
    void setPc(uint16_t pc) {
        if (pageOf(pc) != pageOf(_pc))
            _pageFrame = pageOf(pc);
        _pc = pc;
    }
    // Where the program's PC is physically.
    uint32_t physicalPc() const { return physicalOf(_pageFrame, _pc); }
    // Where the CPU is parked, which is where injection resumes: the
    // program's own fetch, or the NMI handler's.
    uint32_t parkedAt() const { return _inNmi ? _handler : physicalPc(); }
    // A logical address on the parked page maps through its frame. Off
    // that page the frame is unknown, and identity is what a disabled
    // MMU does (section 7.2). Inside the NMI service the CPU is in
    // system mode, whose map is assumed to be identity.
    uint32_t physical(uint16_t logical) const {
        if (!_inNmi && pageOf(logical) == pageOf(_pc))
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
    // Parked inside a mode 3 NMI service (see parkInNmi()).
    bool _inNmi;
    uint16_t _msr;      // the program's MSR: read, or as a mode 3 NMI pushed it
    uint32_t _sspAddr;  // where that push landed: the system SP to return with
    uint32_t _handler;  // the handler fetch the CPU is parked in
    // The debugger runs the CPU in system mode, so _sp is the System
    // Stack Pointer; the User Stack Pointer is a separate register
    // reachable only through LDCTL.
    uint16_t _sp;
    uint16_t _usp;
    uint8_t _iop;  // I/O Page register
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

#ifndef __MEMS_I8086_H__
#define __MEMS_I8086_H__

#include "mems.h"

namespace debugger {
namespace i8086 {

// Little-endian, 1MB, byte-addressed on either bus; the 8086's even byte
// rides AD0-AD7.
struct MemsI8086 final : ExtMemory {
    // |fpu|: the (dis)assembler takes 8087 instructions.
    MemsI8086(bool fpu);

    uint32_t maxAddr() const override { return 0xFFFFF; }

    // A V20/V30 runs native or 8080 code: P's 8080EMU area, from its code
    // segment's base and at most 64K long, holds the 8080 code. D, A,
    // breakpoints and the backtrace take it as 8080 code.
    ProtectArea *protectArea() override { return &_emuArea; }
    bool emulationAt(uint32_t addr) const;
    bool hasEmulationArea() const { return !_emuArea.empty(); }

    // The length of the instruction at |addr|, as the queue status counts
    // it: a WAIT before an 8087 instruction is one of its own.
    uint8_t insnLength(uint32_t addr, bool emu = false) const;
    // Shows the instruction at |addr|, as 8080 code if |emu|; returns the
    // next.
    uint32_t disassembleAs(uint32_t addr, bool emu) const;

    uint32_t assemble(uint32_t addr, const char *line) const override;
    uint32_t disassemble(uint32_t addr, uint8_t numInsn) const override;

private:
    const bool _fpu;
    ProtectArea _emuArea{"8080EMU"};
#ifdef WITH_ASSEMBLER
    libasm::Assembler *_asm8080;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *_dis8080;
#endif

    // 8080 code addresses are 16 bits into a 64K window from here.
    uint32_t windowBase(uint32_t addr) const;

#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

}  // namespace i8086
}  // namespace debugger
#endif /* __MEMS_I8086_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

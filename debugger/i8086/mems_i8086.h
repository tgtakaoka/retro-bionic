#ifndef __MEMS_I8086_H__
#define __MEMS_I8086_H__

#include "mems.h"

namespace debugger {
namespace i8086 {

// Little-endian, 1MB; the even byte rides AD0-AD7.
struct MemsI8086 final : ExtMemory {
    // |fpu|: the (dis)assembler takes 8087 instructions.
    MemsI8086(bool bus8, bool fpu);
    void setBus8(bool bus8) { setWordAccess(!bus8); }

    uint32_t maxAddr() const override { return 0xFFFFF; }

private:
    const bool _fpu;

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

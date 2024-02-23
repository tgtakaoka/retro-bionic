#ifndef __MEMS_MC6809_H__
#define __MEMS_MC6809_H__

#include "mems.h"

namespace debugger {
namespace mc6809 {

struct RegsMc6809;

struct MemsMc6809 : DmaMemory {
    MemsMc6809(RegsMc6809 *regs) : DmaMemory(Endian::ENDIAN_BIG), _regs(regs) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

protected:
    RegsMc6809 *const _regs;

#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

}  // namespace mc6809
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

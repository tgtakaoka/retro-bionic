#ifndef __MEMS_MC6800_H__
#define __MEMS_MC6800_H__

#include "mems.h"

namespace debugger {
namespace mc6800 {

struct RegsMc6800;

struct MemsMc6800 : DmaMemory {
    MemsMc6800(RegsMc6800 &regs) : DmaMemory(Endian::ENDIAN_BIG), _regs(regs) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

protected:
    RegsMc6800 &_regs;

#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

}  // namespace mc6800
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

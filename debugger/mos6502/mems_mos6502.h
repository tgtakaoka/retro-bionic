#ifndef __MEMS_MOS6502_H__
#define __MEMS_MOS6502_H__

#include "mems.h"

namespace debugger {
namespace mos6502 {

struct MemsMos6502 final : DmaMemory {
    MemsMos6502() : DmaMemory(Endian::ENDIAN_LITTLE) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

protected:
#define WITH_ASSEMBLER
#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

extern struct MemsMos6502 Memory;

}  // namespace mos6502
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

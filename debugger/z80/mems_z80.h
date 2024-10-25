#ifndef __MEMS_Z80_H__
#define __MEMS_Z80_H__

#include "mems.h"

namespace debugger {
namespace z80 {

struct RegsZ80;

struct MemsZ80 final : DmaMemory {
    MemsZ80(RegsZ80 &regs) : DmaMemory(Endian::ENDIAN_LITTLE), _regs(regs) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

protected:
    RegsZ80 &_regs;

#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

extern struct MemsZ80 Memory;

}  // namespace z80
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

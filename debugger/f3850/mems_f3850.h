#ifndef __MEMS_F3850_H__
#define __MEMS_F3850_H__

#include "mems.h"

namespace debugger {
namespace f3850 {

struct MemsF3850 final : DmaMemory {
    MemsF3850() : DmaMemory(Endian::ENDIAN_BIG) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

protected:
#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

extern struct MemsF3850 Memory;

}  // namespace f3850
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

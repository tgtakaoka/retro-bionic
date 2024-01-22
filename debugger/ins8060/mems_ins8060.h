#ifndef __MEMS_INS8060_H__
#define __MEMS_INS8060_H__

#include "mems.h"

namespace debugger {
namespace ins8060 {

struct MemsIns8060 final : DmaMemory {
    MemsIns8060() : DmaMemory(Endian::ENDIAN_LITTLE) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

protected:
#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

extern struct MemsIns8060 Memory;

}  // namespace ins8060
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

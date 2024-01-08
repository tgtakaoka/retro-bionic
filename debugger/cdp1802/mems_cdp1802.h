#ifndef __MEMS_CDP1802_H__
#define __MEMS_CDP1802_H__

#include "mems.h"

namespace debugger {
namespace cdp1802 {

struct MemsCdp1802 final : DmaMemory {
    MemsCdp1802() : DmaMemory(Endian::ENDIAN_BIG) {}

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

extern struct MemsCdp1802 Memory;

}  // namespace cdp1802
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

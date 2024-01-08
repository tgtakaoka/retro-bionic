#ifndef __MEMS_Z86_H__
#define __MEMS_Z86_H__

#include "mems.h"

namespace debugger {
namespace z86 {

struct MemsZ86 final : DmaMemory {
    MemsZ86() : DmaMemory(Endian::ENDIAN_BIG) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

protected:
    bool hasRomArea() const override { return true; }

#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

extern struct MemsZ86 Memory;

}  // namespace z86
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

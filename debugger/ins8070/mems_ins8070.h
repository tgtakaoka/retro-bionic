#ifndef __MEMS_INS8070_H__
#define __MEMS_INS8070_H__

#include "mems.h"

namespace debugger {
namespace ins8070 {

struct MemsIns8070 final : DmaMemory {
    MemsIns8070() : DmaMemory(Endian::ENDIAN_LITTLE) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

    static bool is_internal(uint16_t addr) { return addr >= 0xFFC0; }

protected:
#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

extern struct MemsIns8070 Memory;

}  // namespace ins8070
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

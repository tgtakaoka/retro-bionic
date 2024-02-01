#ifndef __MEMS_MC6805_H__
#define __MEMS_MC6805_H__

#include "mems.h"

namespace debugger {
namespace mc6805 {

struct RegsMc6805;

struct MemsMc6805 : DmaMemory {
    MemsMc6805(RegsMc6805 *regs, uint8_t pc_bits)
        : DmaMemory(Endian::ENDIAN_BIG),
          _regs(regs),
          _pc_bits(pc_bits),
          _max_addr((1U << pc_bits) - 1) {}

    uint32_t maxAddr() const override { return _max_addr; }
    uint16_t vecReset() const { return _max_addr - 1; }
    uint16_t vecSwi() const { return _max_addr - 3; }
    virtual bool is_internal(uint16_t addr) const = 0;

    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

protected:
    RegsMc6805 *const _regs;
    const uint8_t _pc_bits;
    const uint16_t _max_addr;

#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

}  // namespace mc6805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

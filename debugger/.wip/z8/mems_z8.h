#ifndef __MEMS_Z8_H__
#define __MEMS_Z8_H__

#include "devs.h"
#include "mems.h"
#include "regs_z8.h"

namespace debugger {
namespace z8 {

struct MemsZ8 : DmaMemory {
    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

protected:
    MemsZ8(RegsZ8 &regs, Devs &devs)
        : DmaMemory(Endian::ENDIAN_BIG), _regs(regs), _devs(devs) {}

    RegsZ8 &_regs;
    Devs &_devs;

#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

}  // namespace z8
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

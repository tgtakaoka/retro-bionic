#ifndef __MEMS_MOS6502_H__
#define __MEMS_MOS6502_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace mos6502 {

struct MemsMos6502 final : ExtMemory {
    MemsMos6502(Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    void longA(bool enable) { _longA = enable; }
    void longI(bool enable) { _longI = enable; }

private:
    Devs *const _devs;
    bool _longA;
    bool _longI;

#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

}  // namespace mos6502
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __MEMS_W65C816_H__
#define __MEMS_W65C816_H__

#include "mems.h"

namespace debugger {
namespace w65c816 {

struct MemsW65c816 final : ExtMemory {
    MemsW65c816() : ExtMemory(Endian::ENDIAN_LITTLE) {}

    uint32_t maxAddr() const override { return 0xFFFFFFU; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    void longA(bool enable) { _longA = enable; }
    void longI(bool enable) { _longI = enable; }

protected:
    bool _longA;
    bool _longI;
#define WITH_ASSEMBLER
#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif
};

extern struct MemsW65c816 Memory;

}  // namespace w65c816
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

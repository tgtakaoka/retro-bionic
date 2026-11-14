#include "mems_pdp11.h"
#include <asm_pdp11.h>
#include <dis_pdp11.h>

namespace debugger {
namespace pdp11 {

MemsPdp11::MemsPdp11() : DmaMemory(Endian::ENDIAN_LITTLE, true) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::pdp11::AsmPdp11();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::pdp11::DisPdp11();
#endif
}

#ifdef WITH_DISASSEMBLER
namespace {
struct CodeMemory final : libasm::DisMemory {
    CodeMemory(const MemsPdp11 *mems, uint32_t addr)
        : libasm::DisMemory(addr), _mems(mems) {}
    bool hasNext() const override { return address() <= _mems->maxAddr(); }
    uint8_t nextByte() override { return _mems->get_code(address()); }

private:
    const MemsPdp11 *const _mems;
};
}  // namespace
#endif

uint_fast8_t MemsPdp11::insnLength(uint32_t addr) const {
#ifdef WITH_DISASSEMBLER
    auto dis = disassembler();
    if (dis) {
        CodeMemory mem(this, addr);
        libasm::Insn insn(addr);
        char operands[80];
        dis->decode(mem, insn, operands, sizeof(operands));
        return insn.length() ? insn.length() : 1;
    }
#endif
    return 0;
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

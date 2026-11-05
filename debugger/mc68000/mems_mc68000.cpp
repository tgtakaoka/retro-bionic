#include "mems_mc68000.h"
#include <asm_mc68000.h>
#include <dis_mc68000.h>

namespace debugger {
namespace mc68000 {

MemsMc68000::MemsMc68000(uint32_t addrMask)
    : ExtMemory(Endian::ENDIAN_BIG, true), _addrMask(addrMask) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mc68000::AsmMc68000();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mc68000::DisMc68000();
    // A7 as the register dump names it, SP
    _disassembler->setOption("sp-alias", "on");
#endif
}

#ifdef WITH_DISASSEMBLER
namespace {
struct CodeMemory final : libasm::DisMemory {
    CodeMemory(const MemsMc68000 *mems, uint32_t addr)
        : libasm::DisMemory(addr), _mems(mems) {}
    bool hasNext() const override { return address() <= _mems->maxAddr(); }
    uint8_t nextByte() override { return _mems->get_code(address()); }

private:
    const MemsMc68000 *const _mems;
};
}  // namespace

uint_fast8_t MemsMc68000::insnLength(uint32_t addr) const {
    auto dis = disassembler();
    if (dis) {
        CodeMemory mem(this, addr);
        libasm::Insn insn(addr);
        char operands[80];
        dis->decode(mem, insn, operands, sizeof(operands));
        if (insn.length() >= 2)
            return insn.length();
    }
    return 2;
}
#endif

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#include "mems_i8086.h"
#include <asm_i8086.h>
#include <dis_i8086.h>

namespace debugger {
namespace i8086 {

MemsI8086::MemsI8086(bool fpu)
    : ExtMemory(Endian::ENDIAN_LITTLE), _fpu(fpu) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8086::AsmI8086();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8086::DisI8086();
#endif
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsI8086::assembler() const {
    auto as = Mems::assembler();
    if (as)
        as->setOption("fpu", _fpu ? "8087" : "off");
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsI8086::disassembler() const {
    auto dis = Mems::disassembler();
    if (dis)
        dis->setOption("fpu", _fpu ? "8087" : "off");
    return dis;
}

namespace {
struct CodeMemory final : libasm::DisMemory {
    CodeMemory(const Mems *mems, uint32_t addr)
        : libasm::DisMemory(addr), _mems(mems) {}
    bool hasNext() const override { return address() <= 0xFFFFF; }
    uint8_t nextByte() override { return _mems->get_code(address()); }

private:
    const Mems *_mems;
};
}  // namespace
#endif

uint8_t MemsI8086::insnLength(uint32_t addr) const {
#ifdef WITH_DISASSEMBLER
    auto dis = disassembler();
    if (dis) {
        CodeMemory mem(this, addr);
        libasm::Insn insn(addr);
        char operands[80];
        dis->decode(mem, insn, operands, sizeof(operands));
        const auto at = insn.hasContinue() ? insn.continueOffset() : 0;
        const auto len = at ? at : insn.length();
        return len ? len : 1;
    }
#endif
    return 1;
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

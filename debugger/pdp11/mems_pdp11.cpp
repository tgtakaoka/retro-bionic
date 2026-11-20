#include "mems_pdp11.h"
#include <asm_pdp11.h>
#include <dis_pdp11.h>

namespace debugger {
namespace pdp11 {

template <typename MEM>
MemsPdp11<MEM>::MemsPdp11(uint32_t maxAddr)
    : MEM(Endian::ENDIAN_LITTLE, true), _maxAddr(maxAddr) {
#ifdef WITH_ASSEMBLER
    this->_assembler = new libasm::pdp11::AsmPdp11();
#endif
#ifdef WITH_DISASSEMBLER
    this->_disassembler = new libasm::pdp11::DisPdp11();
#endif
}

#ifdef WITH_DISASSEMBLER
template <typename MEM>
libasm::Disassembler *MemsPdp11<MEM>::disassembler() const {
    auto dis = MEM::disassembler();
    if (dis) {
        dis->setOption("fdmode", _fd ? "double" : "single");
        dis->setOption("flmode", _fl ? "long" : "short");
    }
    return dis;
}

namespace {
struct CodeMemory final : libasm::DisMemory {
    CodeMemory(const Mems *mems, uint32_t addr)
        : libasm::DisMemory(addr), _mems(mems) {}
    bool hasNext() const override { return address() <= _mems->maxAddr(); }
    uint8_t nextByte() override { return _mems->get_code(address()); }

private:
    const Mems *const _mems;
};
}  // namespace
#endif

template <typename MEM>
uint_fast8_t MemsPdp11<MEM>::insnLength(uint32_t addr) const {
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

template struct MemsPdp11<DmaMemory>;
template struct MemsPdp11<ExtMemory>;

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

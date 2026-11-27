#include "mems_z8000.h"
#include <asm_z8000.h>
#include <dis_z8000.h>
#include "debugger.h"
#include "inst_z8000.h"
#include "regs_z8000.h"

namespace debugger {
namespace z8000 {

MemsZ8000::MemsZ8000(bool segmented)
    : ExtMemory(Endian::ENDIAN_BIG, true),
      _segmented(segmented),
      _maxAddr(segmented ? 0x7FFFFF : UINT16_MAX) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::z8000::AsmZ8000();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::z8000::DisZ8000();
    // R15 (RR14 when segmented) as the register dump names it
    _disassembler->setOption("sp-alias", "on");
#endif
}

bool MemsZ8000::nonsegmented() const {
    if (!_segmented)
        return false;
    if (_decode != DECODE_FCW)
        return _decode == DECODE_NONSEG;
    return _regs && (_regs->fcw() & InstZ8000::FCW_SEG) == 0;
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsZ8000::assembler() const {
    auto as = ExtMemory::assembler();
    if (as && nonsegmented())
        as->setCpu("Z8002");
    // Z8070 instructions, always: with no EPU on the board they trap to an
    // emulator, so the FCW's EPA bit is off whenever they run.
    if (as)
        as->setOption("fpu", "on");
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsZ8000::disassembler() const {
    auto dis = ExtMemory::disassembler();
    if (dis && nonsegmented())
        dis->setCpu("Z8002");
    if (dis)
        dis->setOption("fpu", "on");
    return dis;
}

namespace {
// Code at |base| + address(): Z8002 code takes 16-bit addresses, so a
// Z8001 running nonsegmented decodes at offsets in its segment.
struct CodeMemory final : libasm::DisMemory {
    CodeMemory(const MemsZ8000 *mems, uint32_t base, uint32_t addr,
            uint32_t max)
        : libasm::DisMemory(addr), _mems(mems), _base(base), _max(max) {}
    bool hasNext() const override { return address() <= _max; }
    uint8_t nextByte() override { return _mems->get_code(_base + address()); }

private:
    const MemsZ8000 *_mems;
    const uint32_t _base;
    const uint32_t _max;
};

void printError(const libasm::ErrorAt &error) {
    cli.print("Error: ");
    cli.printStr_P(error.errorText_P());
    if (*error.errorAt()) {
        cli.print(" at '");
        cli.printStr(error.errorAt());
        cli.print('\'');
    }
    cli.println();
}
}  // namespace

uint_fast8_t MemsZ8000::insnWords(uint32_t addr, Decode mode) const {
    const auto saved = _decode;
    _decode = mode;
    const auto nonseg = nonsegmented();
    auto dis = disassembler();
    _decode = saved;
    if (dis == nullptr)
        return 0;
    const auto base = nonseg ? addr & ~(SEG_SIZE - 1) : 0;
    CodeMemory mem(this, base, addr - base, nonseg ? UINT16_MAX : _maxAddr);
    libasm::Insn insn(addr - base);
    char operands[80];
    dis->decode(mem, insn, operands, sizeof(operands));
    return insn.getError() ? 0 : insn.length() / 2;
}

uint32_t MemsZ8000::disassemble(uint32_t addr, uint8_t numInsn) const {
    const auto base = addr & ~(SEG_SIZE - 1);
    if (base == 0 || !nonsegmented())
        return ExtMemory::disassemble(addr, numInsn);
    auto dis = disassembler();
    if (dis == nullptr)
        return addr;
    dis->setOption("uppercase", "true");
    for (auto n = 0; n < numInsn; ++n) {
        CodeMemory mem(this, base, addr - base, UINT16_MAX);
        libasm::Insn insn(addr - base);
        char operands[80];
        dis->decode(mem, insn, operands, sizeof(operands));
        const auto len = insn.length();
        printInsn(dis, insn, operands, 0, len, true, base);
        if (insn.getError())
            printError(insn);
        addr = base + ((addr - base + (len ? len : 1)) & UINT16_MAX);
    }
    return addr;
}
#endif

}  // namespace z8000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

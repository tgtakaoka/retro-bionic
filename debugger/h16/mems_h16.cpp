#include "mems_h16.h"
#include "regs_h16.h"
#include <asm_h16.h>
#include <dis_h16.h>
#include <string.h>

namespace debugger {
namespace h16 {

MemsH16::MemsH16() : ExtMemory(Endian::ENDIAN_BIG) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::h16::AsmH16();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::h16::DisH16();
    // R15 as the register dump names it, SSP or USP
    _disassembler->setOption("sp-alias", "on");
#endif
}

#ifdef WITH_DISASSEMBLER
namespace {
struct CodeMemory final : libasm::DisMemory {
    CodeMemory(const Mems *mems, uint32_t addr)
        : libasm::DisMemory(addr), _mems(mems) {}
    bool hasNext() const override { return address() <= MemsH16::ADDR_MASK; }
    uint8_t nextByte() override { return _mems->get_code(address()); }

private:
    const Mems *_mems;
};
}  // namespace
#endif

uint_fast8_t MemsH16::insnLength(uint32_t addr) const {
#ifdef WITH_DISASSEMBLER
    CodeMemory mem(this, addr);
    libasm::Insn insn(addr);
    char operands[80];
    _disassembler->decode(mem, insn, operands, sizeof(operands));
    if (insn.length())
        return insn.length();
#endif
    return 1;
}

uint32_t MemsH16::disassembleOne(uint32_t addr, bool maybe) const {
#ifdef WITH_DISASSEMBLER
    if (maybe) {
        _disassembler->setOption("uppercase", "true");
        CodeMemory mem(this, addr);
        libasm::Insn insn(addr);
        char operands[80];
        _disassembler->decode(mem, insn, operands, sizeof(operands) - 3);
        strcat(operands, "  ?");
        printInsn(_disassembler, insn, operands, 0, insn.length(), true);
        return addr + (insn.length() ? insn.length() : 1);
    }
#endif
    return Mems::disassemble(addr, 1);
}

uint16_t MemsH16::get_data(uint32_t addr) const {
    if (_regs && _regs->isInternal(addr))
        return _regs->internalRead(addr);
    return get_prog(addr);
}

void MemsH16::put_data(uint32_t addr, uint16_t data) const {
    if (_regs && _regs->isInternal(addr)) {
        _regs->internalWrite(addr, data);
    } else {
        put_prog(addr, data);
    }
}

}  // namespace h16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

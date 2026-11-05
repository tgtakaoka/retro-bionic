#include "mems_i8086.h"
#include <asm_i8080.h>
#include <asm_i8086.h>
#include <dis_i8080.h>
#include <dis_i8086.h>
#include "debugger.h"

namespace debugger {
namespace i8086 {

MemsI8086::MemsI8086(const char *fpu, uint32_t addrMask)
    : ExtMemory(Endian::ENDIAN_LITTLE), _fpu(fpu), _addrMask(addrMask) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8086::AsmI8086();
    _asm8080 = new libasm::i8080::AsmI8080();
    static_cast<libasm::Assembler *>(_asm8080)->setCpu("V30EMU");
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8086::DisI8086();
    _dis8080 = new libasm::i8080::DisI8080();
    static_cast<libasm::Disassembler *>(_dis8080)->setCpu("V30EMU");
    _dis8080->setOption("uppercase", "true");
#endif
}

bool MemsI8086::emulationAt(uint32_t addr) const {
    return !_emuArea.empty() && addr >= _emuArea.begin() &&
           addr <= _emuArea.end() && addr - _emuArea.begin() < 0x10000;
}

// |addr|'s window: the 8080EMU area's if it falls in it, else its own 64K.
uint32_t MemsI8086::windowBase(uint32_t addr) const {
    return emulationAt(addr) ? _emuArea.begin() : (addr & 0xF0000);
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsI8086::assembler() const {
    auto as = Mems::assembler();
    if (as)
        as->setOption("fpu", _fpu);
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsI8086::disassembler() const {
    auto dis = Mems::disassembler();
    if (dis)
        dis->setOption("fpu", _fpu);
    return dis;
}

namespace {
// Code at |base| + address(); 8080 code takes 16-bit addresses.
struct CodeMemory final : libasm::DisMemory {
    CodeMemory(const Mems *mems, uint32_t base, uint32_t addr, uint32_t max)
        : libasm::DisMemory(addr), _mems(mems), _base(base), _max(max) {}
    bool hasNext() const override { return address() <= _max; }
    uint8_t nextByte() override {
        return _mems->get_code((_base + address()) & _mems->maxAddr());
    }

private:
    const Mems *_mems;
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

uint32_t MemsI8086::disassembleAs(uint32_t addr, bool emu) const {
    if (!emu)
        return Mems::disassemble(addr, 1);
    const auto base = windowBase(addr);
    libasm::Insn insn((addr - base) & 0xFFFF);
    CodeMemory mem(this, base, insn.address(), 0xFFFF);
    char operands[80];
    _dis8080->decode(mem, insn, operands, sizeof(operands));
    printInsn(_dis8080, insn, operands, 0, insn.length(), true, base);
    if (insn.getError())
        printError(insn);
    return base + insn.address() + (insn.length() ? insn.length() : 1);
}

uint32_t MemsI8086::disassemble(uint32_t addr, uint8_t numInsn) const {
    while (numInsn--)
        addr = disassembleAs(addr, emulationAt(addr));
    return addr;
}
#endif

uint32_t MemsI8086::assemble(uint32_t addr, const char *line) const {
#if defined(WITH_ASSEMBLER) && defined(WITH_DISASSEMBLER)
    if (emulationAt(addr)) {
        const auto base = windowBase(addr);
        libasm::Insn insn((addr - base) & 0xFFFF);
        _asm8080->encode(line, insn);
        if (insn.getError()) {
            printError(insn);
            return addr;
        }
        put_code(base + insn.address(), insn.bytes(), insn.length());
        disassembleAs(base + insn.address(), true);
        return base + insn.address() + insn.length();
    }
#endif
    return Mems::assemble(addr, line);
}

#ifdef WITH_DISASSEMBLER
uint8_t MemsI8086::insnLength(uint32_t addr, bool emu) const {
    auto dis = emu ? _dis8080 : disassembler();
    if (dis) {
        const auto base = emu ? windowBase(addr) : 0;
        const uint32_t pc = emu ? (addr - base) & 0xFFFF : addr;
        CodeMemory mem(this, base, pc, emu ? 0xFFFF : _addrMask);
        libasm::Insn insn(pc);
        char operands[80];
        dis->decode(mem, insn, operands, sizeof(operands));
        const auto at = insn.hasContinue() ? insn.continueOffset() : 0;
        const auto len = at ? at : insn.length();
        return len ? len : 1;
    }
    return 1;
}
#endif

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

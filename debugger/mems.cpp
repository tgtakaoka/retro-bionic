#include "mems.h"
#include "debugger.h"

#ifdef WITH_ASSEMBLER
#include <asm_base.h>
#endif
#ifdef WITH_DISASSEMBLER
#include <dis_base.h>
#endif
#include <dis_memory.h>

namespace debugger {

namespace {

struct DisMemory final : libasm::DisMemory {
    DisMemory(const Mems *mems) : libasm::DisMemory(0), _mems(mems) {}
    const Mems *_mems;

    bool hasNext() const override { return address() < _mems->maxAddr(); }
    void setAddress(uint32_t addr) { resetAddress(addr); }
    uint8_t nextByte() override { return _mems->raw_read(address()); }
};

uint8_t DMA_MEMORY[DmaMemory::MEM_SIZE] DMAMEM;
uint8_t EXT_MEMORY[ExtMemory::MEM_SIZE] EXTMEM;

}  // namespace

uint16_t DmaMemory::raw_read(uint32_t addr) const {
    return addr < MEM_SIZE ? DMA_MEMORY[addr] : 0;
}

void DmaMemory::raw_write(uint32_t addr, uint16_t data) const {
    if (addr < MEM_SIZE)
        DMA_MEMORY[addr] = data;
}

uint16_t ExtMemory::raw_read(uint32_t addr) const {
    return addr < MEM_SIZE ? EXT_MEMORY[addr] : 0;
}

void ExtMemory::raw_write(uint32_t addr, uint16_t data) const {
    if (addr < MEM_SIZE)
        EXT_MEMORY[addr] = data;
}

Mems::Mems(Endian endian)
    : _endian(endian)
#ifdef WITH_ASSEMBLER
      ,
      _assembler(nullptr)
#endif
#ifdef WITH_DISASSEMBLER
      ,
      _disassembler(nullptr)
#endif
{
}

Mems::~Mems() {
#ifdef WITH_ASSEMBLER
    delete _assembler;
#endif
#ifdef WITH_DISASSEMBLER
    delete _disassembler;
#endif
}

uint16_t Mems::raw_read16(uint32_t addr) const {
    return _endian == ENDIAN_BIG ? raw_read16be(addr) : raw_read16le(addr);
}

uint16_t Mems::raw_read16be(uint32_t addr) const {
    auto data = raw_read(addr + 0) << 8;
    data |= raw_read(addr + 1) & 0xFF;
    return data;
}

uint16_t Mems::raw_read16le(uint32_t addr) const {
    auto data = raw_read(addr + 1) << 8;
    data |= raw_read(addr + 0) & 0xFF;
    return data;
}

void Mems::raw_write16(uint32_t addr, uint16_t data) const {
    if (_endian == ENDIAN_BIG)
        raw_write16be(addr, data);
    else
        raw_write16le(addr, data);
}

void Mems::raw_write16be(uint32_t addr, uint16_t data) const {
    raw_write(addr + 0, hi(data));
    raw_write(addr + 1, lo(data));
}

void Mems::raw_write16le(uint32_t addr, uint16_t data) const {
    raw_write(addr + 0, lo(data));
    raw_write(addr + 1, hi(data));
}

void Mems::put(uint32_t addr, const uint8_t *buffer, uint8_t len) const {
    for (auto i = 0; i < len; ++i) {
        put(addr + i, buffer[i]);
    }
}

void Mems::RomArea::set(uint32_t begin, uint32_t end) {
    _begin = begin;
    _end = end;
}

bool Mems::RomArea::readOnly(uint32_t addr) const {
    return addr >= _begin && addr <= _end;
}

void Mems::RomArea::print() const {
    cli.print("ROM area: ");
    if (_begin < _end) {
        cli.printHex(_begin, 4);
        cli.print('-');
        cli.printlnHex(_end, 4);
    } else {
        cli.println("none");
    }
}

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *Mems::disassembler() const {
    auto dis = _disassembler;
    if (dis)
        dis->setCpu(Debugger.target().cpu());
    return dis;
}
#endif

uint32_t Mems::disassemble(uint32_t addr, uint8_t numInsn) const {
#ifdef WITH_DISASSEMBLER
    auto dis = disassembler();
    if (dis) {
        const auto nameWidth = dis->config().nameMax() + 1;
        const auto addrDigit = ((dis->config().addressWidth() + 3) & -4) / 4;
        dis->setOption("uppercase", "true");
        DisMemory mem(this);
        uint16_t num = 0;
        while (num < numInsn) {
            char operands[80];
            libasm::Insn insn(addr);
            mem.setAddress(addr);
            dis->decode(mem, insn, operands, sizeof(operands));
            cli.printHex(insn.address(), addrDigit);
            cli.print(':');
            // TODO: support OPCODE_12BIT, OPCODE_16BIT, ADDRESS_WORD etc.
            // See libasm's arduino_example.h
            for (auto i = 0; i < insn.length(); i++) {
                cli.printHex(insn.bytes()[i], 2);
                cli.print(' ');
            }
            for (auto i = insn.length(); i < 5; i++) {
                cli.print("   ");
            }
            cli.printStr(insn.name(), -nameWidth);
            cli.printlnStr(operands);
            if (insn.getError()) {
                cli.print("Error: ");
                cli.printStr_P(insn.errorText_P());
                if (*insn.errorAt()) {
                    cli.print(" at '");
                    cli.printStr(insn.errorAt());
                    cli.print('\'');
                }
                cli.println();
                if (insn.getError() == libasm::NO_MEMORY)
                    break;
            }
            addr += insn.length();
            ++num;
        }
    }
#endif
    return addr;
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *Mems::assembler() const {
    auto as = _assembler;
    if (as)
        as->setCpu(Debugger.target().cpu());
    return as;
}
#endif

uint32_t Mems::assemble(uint32_t addr, const char *line) const {
#ifdef WITH_ASSEMBLER
    auto as = assembler();
    if (as) {
        libasm::Insn insn(addr);
        as->encode(line, insn);
        if (insn.getError()) {
            cli.print("Error: ");
            cli.print(insn.errorText_P());
            if (*insn.errorAt()) {
                cli.print(" at '");
                cli.printStr(insn.errorAt());
                cli.print('\'');
            }
            cli.println();
        } else {
            put(insn.address(), insn.bytes(), insn.length());
            disassemble(insn.address(), 1);
            addr += insn.length();
        }
    }
#endif
    return addr;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

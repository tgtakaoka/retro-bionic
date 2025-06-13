#include "mems_pdp8.h"
#include <asm_pdp8.h>
#include <dis_pdp8.h>

namespace debugger {
namespace pdp8 {

MemsPdp8::MemsPdp8(uint8_t addr_bit)
    : DmaMemory(Endian::ENDIAN_BIG), _max_addr((1 << addr_bit) - 1) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::pdp8::AsmPdp8();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::pdp8::DisPdp8();
#endif
}

void MemsPdp8::isPrint(const uint8_t *data, char buf[2]) const {
    buf[0] = data[0] ? '.' : ' ';
    buf[1] = isprint(data[1]) ? data[1] : '.';
}

ControlPanel::ControlPanel(MemsPdp8 *mems)
    : DmaMemory(Endian::ENDIAN_BIG), _mems(mems) {
#ifdef WITH_ASSEMBLER
    _assembler = mems->getAsm();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = mems->getDis();
#endif
}

uint16_t ControlPanel::read(uint32_t addr) const {
    return read_word(addr + OFFSET) & 07777;
}

void ControlPanel::write(uint32_t addr, uint16_t data) const {
    write_word(addr + OFFSET, data);
}

}  // namespace pdp8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

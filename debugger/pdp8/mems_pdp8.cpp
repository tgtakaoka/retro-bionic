#include <asm_pdp8.h>
#include <dis_pdp8.h>

#include "mems_pdp8.h"

namespace debugger {
namespace pdp8 {

MemsPdp8::MemsPdp8(uint8_t addr_bit, Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG),
      _devs(devs),
      _max_addr((1 << addr_bit) - 1) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::pdp8::AsmPdp8();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::pdp8::DisPdp8();
#endif
}

namespace {
char dec2Ascii(uint8_t dec) {
    dec &= 077;
    const auto c = dec < 040 ? dec + 0x40 : dec;
    return isprint(c) ? c : '.';
}
void is_print(const uint16_t word, char buf[2]) {
    if ((word & ~0177) == 0200) {
        buf[0] = ' ';
        buf[1] = word & 0177;
    } else {
        buf[0] = dec2Ascii(word >> 6);
        buf[1] = dec2Ascii(word);
    }
}

}  // namespace

void MemsPdp8::isPrint(const uint8_t *data, char buf[2]) const {
    is_print(uint16(data[0], data[1]) & 07777, buf);
}

uint16_t MemsPdp8::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr)
                                   : (raw_read16(addr * 2) & 07777);
}

void MemsPdp8::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        raw_write16(addr * 2, data);
    }
}

uint16_t MemsPdp8::get(uint32_t addr, const char *) const {
    return read(addr);
}

void MemsPdp8::put(uint32_t addr, uint16_t data, const char *) const {
    write(addr, data);
}

ControlPanel::ControlPanel(MemsPdp8 *mems) : DmaMemory(Endian::ENDIAN_BIG) {
#ifdef WITH_ASSEMBLER
    _assembler = mems->getAsm();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = mems->getDis();
#endif
}

void ControlPanel::isPrint(const uint8_t *data, char buf[2]) const {
    is_print(uint16(data[0], data[1]) & 07777, buf);
}

uint16_t ControlPanel::read(uint32_t addr) const {
    return raw_read16(addr + OFFSET) & 07777;
}

void ControlPanel::write(uint32_t addr, uint16_t data) const {
    raw_write16(addr + OFFSET, data);
}
}  // namespace pdp8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

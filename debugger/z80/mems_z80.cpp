#include "mems_z80.h"
#include <asm_z80.h>
#include <ctype.h>
#include <dis_z80.h>
#include "regs_z80.h"

namespace debugger {
namespace z80 {

MemsZ80::MemsZ80(RegsZ80 *regs)
    : DmaMemory(Endian::ENDIAN_LITTLE), _regs(regs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::z80::AsmZ80();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::z80::DisZ80();
#endif
}

uint16_t MemsZ80::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read(addr);
    if (toupper(*space) == 'I' && addr < 0x100)
        return _regs->read_io(addr);
    return 0;
}

void MemsZ80::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write(addr, data);
    } else if (toupper(*space) == 'I' && addr < 0x100) {
        _regs->write_io(addr, data);
    }
}

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

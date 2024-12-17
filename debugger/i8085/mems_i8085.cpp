#include <ctype.h>

#include <asm_i8080.h>
#include <dis_i8080.h>

#include "mems_i8085.h"
#include "regs_i8085.h"

namespace debugger {
namespace i8085 {

MemsI8085::MemsI8085(RegsI8085 *regs)
    : DmaMemory(Endian::ENDIAN_LITTLE), _regs(regs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8080::AsmI8080();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8080::DisI8080();
#endif
}

uint16_t MemsI8085::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read(addr);
    if (toupper(*space) == 'I' && addr < 0x100)
        return _regs->read_io(addr);
    return 0;
}

void MemsI8085::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write(addr, data);
    } else if (toupper(*space) == 'I' && addr < 0x100) {
        _regs->write_io(addr, data);
    }
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

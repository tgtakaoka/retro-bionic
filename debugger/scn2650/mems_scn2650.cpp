#include "mems_scn2650.h"
#include <asm_scn2650.h>
#include <ctype.h>
#include <dis_scn2650.h>
#include "regs_scn2650.h"

namespace debugger {
namespace scn2650 {

MemsScn2650::MemsScn2650(RegsScn2650 *regs)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::scn2650::AsmScn2650();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::scn2650::DisScn2650();
#endif
}

uint16_t MemsScn2650::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read(addr);
    if (toupper(*space) == 'I' && addr < 0x100)
        return _regs->read_io(addr);
    return 0;
}

void MemsScn2650::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write(addr, data);
    } else if (toupper(*space) == 'I' && addr < 0x100) {
        _regs->write_io(addr, data);
    }
}

}  // namespace scn2650
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

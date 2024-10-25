#include "mems_z80.h"
#include <asm_z80.h>
#include <ctype.h>
#include <dis_z80.h>
#include "regs_z80.h"

namespace debugger {
namespace z80 {

uint16_t MemsZ80::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read(addr);
    if (toupper(*space) == 'I' && addr < 0x100)
        return _regs.read_io(addr);
    return 0;
}

void MemsZ80::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write(addr, data);
    } else if (toupper(*space) == 'I' && addr < 0x100) {
        _regs.write_io(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsZ80::assembler() const {
    static auto as = new libasm::z80::AsmZ80();
    as->setCpu(_regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsZ80::disassembler() const {
    static auto dis = new libasm::z80::DisZ80();
    dis->setCpu(_regs.cpu());
    return dis;
}
#endif

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

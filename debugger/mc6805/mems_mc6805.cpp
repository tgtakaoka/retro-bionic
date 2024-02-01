#include "mems_mc6805.h"
#include <asm_mc6805.h>
#include <dis_mc6805.h>
#include "regs_mc6805.h"

namespace debugger {
namespace mc6805 {

uint16_t MemsMc6805::get(uint32_t addr, const char *space) const {
    (void)space;
    if (is_internal(addr)) {
        return _regs->internal_read(addr);
    } else {
        return read(addr);
    }
}

void MemsMc6805::put(uint32_t addr, uint16_t data, const char *space) const {
    if (is_internal(addr)) {
        _regs->internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsMc6805::assembler() const {
    static auto as = new libasm::mc6805::AsmMc6805();
    as->setCpu(_regs->cpu());
    as->setPcBits(_pc_bits);
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsMc6805::disassembler() const {
    static auto dis = new libasm::mc6805::DisMc6805();
    dis->setCpu(_regs->cpu());
    dis->setPcBits(_pc_bits);
    return dis;
}
#endif

}  // namespace mc6805
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

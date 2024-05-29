#include "mems_mc6800.h"
#include <asm_mc6800.h>
#include <dis_mc6800.h>
#include "devs_mc6800.h"
#include "regs_mc6800.h"

namespace debugger {
namespace mc6800 {

uint16_t MemsMc6800::read(uint32_t addr) const {
    return Devices.isSelected(addr) ? Devices.read(addr) : raw_read(addr);
}

void MemsMc6800::write(uint32_t addr, uint16_t data) const {
    if (Devices.isSelected(addr)) {
        Devices.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsMc6800::assembler() const {
    static auto as = new libasm::mc6800::AsmMc6800();
    as->setCpu(_regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsMc6800::disassembler() const {
    static auto dis = new libasm::mc6800::DisMc6800();
    dis->setCpu(_regs.cpu());
    return dis;
}
#endif

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#include <asm_ins8070.h>
#include <dis_ins8070.h>

#include "devs_ins8070.h"
#include "mems_ins8070.h"
#include "pins_ins8070.h"
#include "regs_ins8070.h"

namespace debugger {
namespace ins8070 {

struct MemsIns8070 Memory;

uint16_t MemsIns8070::read(uint32_t addr) const {
    return Devs.isSelected(addr) ? Devs.read(addr) : raw_read(addr);
}

void MemsIns8070::write(uint32_t addr, uint16_t data) const {
    if (Devs.isSelected(addr)) {
        Devs.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsIns8070::get(uint32_t addr, const char *space) const {
    (void)space;
    return is_internal(addr) ? RegsIns8070::internal_read(addr) : read(addr);
}

void MemsIns8070::put(uint32_t addr, uint16_t data, const char *space) const {
    (void)space;
    if (is_internal(addr)) {
        Regs.internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsIns8070::assembler() const {
    static auto as = new libasm::ins8070::AsmIns8070();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsIns8070::disassembler() const {
    static auto dis = new libasm::ins8070::DisIns8070();
    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

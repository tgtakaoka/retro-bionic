#include <asm_ins8060.h>
#include <dis_ins8060.h>

#include "devs_ins8060.h"
#include "mems_ins8060.h"
#include "regs_ins8060.h"

namespace debugger {
namespace ins8060 {

struct MemsIns8060 Memory;

uint16_t MemsIns8060::read(uint32_t addr) const {
    if (Devs.isSelected(addr))
        return Devs.read(addr);
    return raw_read(addr);
}

void MemsIns8060::write(uint32_t addr, uint16_t data) const {
    if (Devs.isSelected(addr)) {
        Devs.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsIns8060::assembler() const {
    static auto as = new libasm::ins8060::AsmIns8060();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsIns8060::disassembler() const {
    static auto dis = new libasm::ins8060::DisIns8060();
    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

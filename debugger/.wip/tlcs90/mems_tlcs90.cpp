#include "mems_tlcs90.h"
#include <asm_tlcs90.h>
#include <dis_tlcs90.h>
#include "devs_tlcs90.h"
#include "pins_tlcs90.h"
#include "regs_tlcs90.h"

namespace debugger {
namespace tlcs90 {

struct MemsTlcs90 Memory;

uint16_t MemsTlcs90::read(uint32_t addr) const {
    return Devs.isSelected(addr) ? Devs.read(addr) : raw_read(addr);
}

void MemsTlcs90::write(uint32_t addr, uint16_t data) const {
    if (Devs.isSelected(addr)) {
        Devs.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsTlcs90::get(uint32_t addr, const char *space) const {
    (void)space;
    return is_internal(addr) ? RegsTlcs90::internal_read(addr) : read(addr);
}

void MemsTlcs90::put(uint32_t addr, uint16_t data, const char *space) const {
    (void)space;
    if (is_internal(addr)) {
        RegsTlcs90::internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsTlcs90::assembler() const {
    static auto as = new libasm::tlcs90::AsmTlcs90();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsTlcs90::disassembler() const {
    static auto dis = new libasm::tlcs90::DisTlcs90();
    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

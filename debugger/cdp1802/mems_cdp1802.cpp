#include <asm_cdp1802.h>
#include <dis_cdp1802.h>

#include "devs_cdp1802.h"
#include "mems_cdp1802.h"
#include "regs_cdp1802.h"

namespace debugger {
namespace cdp1802 {

struct MemsCdp1802 Memory;

uint16_t MemsCdp1802::read(uint32_t addr) const {
    return Devs.isSelected(addr) ? Devs.read(addr) : raw_read(addr);
}

void MemsCdp1802::write(uint32_t addr, uint16_t data) const {
    if (Devs.isSelected(addr)) {
        Devs.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsCdp1802::assembler() const {
    static auto as = new libasm::cdp1802::AsmCdp1802();
    as->setCpu(Regs.cpu());
    as->setOption("use-register", "on");
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsCdp1802::disassembler() const {
    static auto dis = new libasm::cdp1802::DisCdp1802();
    dis->setCpu(Regs.cpu());
    dis->setOption("use-register", "on");
    return dis;
}
#endif

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

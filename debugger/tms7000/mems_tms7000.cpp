#include "mems_tms7000.h"
#include <asm_tms7000.h>
#include <ctype.h>
#include <dis_tms7000.h>
#include "devs_tms7000.h"
#include "regs_tms7000.h"

namespace debugger {
namespace tms7000 {

struct MemsTms7000 Memory;

uint16_t MemsTms7000::read(uint32_t addr) const {
    return Devs.isSelected(addr) ? Devs.read(addr) : raw_read(addr);
}

void MemsTms7000::write(uint32_t addr, uint16_t data) const {
    if (Devs.isSelected(addr)) {
        Devs.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsTms7000::get(uint32_t addr, const char *space) const {
    if (Devs.isSelected(addr))
        return Devs.read(addr);
    return addr < 0x0200 ? Regs.read_internal(addr) : raw_read(addr);
}

void MemsTms7000::put(uint32_t addr, uint16_t data, const char *space) const {
    if (Devs.isSelected(addr)) {
        Devs.write(addr, data);
    } else if (addr < 0x0200) {
        Regs.write_internal(addr, data);
    } else {
        raw_write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsTms7000::assembler() const {
    static auto as = new libasm::tms7000::AsmTms7000();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsTms7000::disassembler() const {
    static auto dis = new libasm::tms7000::DisTms7000();
    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

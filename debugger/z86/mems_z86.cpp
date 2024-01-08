#include <ctype.h>

#include <asm_z8.h>
#include <dis_z8.h>

#include "devs_z86.h"
#include "mems_z86.h"
#include "regs_z86.h"

namespace debugger {
namespace z86 {

struct MemsZ86 Memory;

uint16_t MemsZ86::read(uint32_t addr) const {
    return Devs.isSelected(addr) ? Devs.read(addr) : raw_read(addr);
}

void MemsZ86::write(uint32_t addr, uint16_t data) const {
    if (Devs.isSelected(addr)) {
        Devs.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsZ86::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read(addr);
    if (toupper(*space) == 'R' && addr < 0x100)
        return Regs.read_reg(addr);
    return 0;
}

void MemsZ86::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write(addr, data);
    } else if (toupper(*space) == 'R' && addr < 0x100) {
        Regs.write_reg(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsZ86::assembler() const {
    static auto as = new libasm::z8::AsmZ8();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsZ86::disassembler() const {
    static auto dis = new libasm::z8::DisZ8();
    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace z86
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

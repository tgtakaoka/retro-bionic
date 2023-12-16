#include <ctype.h>

#include <asm_f3850.h>
#include <dis_f3850.h>

#include "mems_f3850.h"
#include "regs_f3850.h"

namespace debugger {
namespace f3850 {

struct MemsF3850 Memory;

uint16_t MemsF3850::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return raw_read(addr);
    if (toupper(*space) == 'I' && addr < 0x10)
        return Regs.read_io(addr);
    if (toupper(*space) == 'R' && addr < 0x40)
        return Regs.read_reg(addr);
    return 0;
}

void MemsF3850::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        raw_write(addr, data);
    } else if (toupper(*space) == 'I' && addr < 0x10) {
        Regs.write_io(addr, data);
    } else if (toupper(*space) == 'R' && addr < 0x40) {
        Regs.write_reg(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsF3850::assembler() const {
    static auto as = new libasm::f3850::AsmF3850();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsF3850::disassembler() const {
    static auto dis = new libasm::f3850::DisF3850();
    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace f3850
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#include <ctype.h>

#include <asm_scn2650.h>
#include <dis_scn2650.h>

#include "mems_scn2650.h"
#include "regs_scn2650.h"

namespace debugger {
namespace scn2650 {

struct MemsScn2650 Memory;

uint16_t MemsScn2650::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read(addr);
    if (toupper(*space) == 'I' && addr < 0x100)
        return Regs.read_io(addr);
    return 0;
}

void MemsScn2650::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write(addr, data);
    } else if (toupper(*space) == 'I' && addr < 0x100) {
        Regs.write_io(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsScn2650::assembler() const {
    static auto as = new libasm::scn2650::AsmScn2650();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsScn2650::disassembler() const {
    static auto dis = new libasm::scn2650::DisScn2650();
    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace scn2650
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

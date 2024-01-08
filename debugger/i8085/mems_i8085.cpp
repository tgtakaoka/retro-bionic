#include <ctype.h>

#include <asm_i8080.h>
#include <dis_i8080.h>

#include "mems_i8085.h"
#include "regs_i8085.h"

namespace debugger {
namespace i8085 {

struct MemsI8085 Memory;

uint16_t MemsI8085::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read(addr);
    if (toupper(*space) == 'I' && addr < 0x100)
        return Regs.read_io(addr);
    return 0;
}

void MemsI8085::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write(addr, data);
    } else if (toupper(*space) == 'I' && addr < 0x100) {
        Regs.write_io(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsI8085::assembler() const {
    static auto as = new libasm::i8080::AsmI8080();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsI8085::disassembler() const {
    static auto dis = new libasm::i8080::DisI8080();
    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

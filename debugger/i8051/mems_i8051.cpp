#include "mems_i8051.h"
#include <asm_i8051.h>
#include <ctype.h>
#include <dis_i8051.h>
#include "devs_i8051.h"
#include "regs_i8051.h"

namespace debugger {
namespace i8051 {

struct ProgI8051 ProgMemory;
struct DataI8051 DataMemory;

uint16_t DataI8051::read(uint32_t addr) const {
    return Devs.isSelected(addr) ? Devs.read(addr) : raw_read(addr);
}

void DataI8051::write(uint32_t addr, uint16_t data) const {
    if (Devs.isSelected(addr)) {
        Devs.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t ProgI8051::get(uint32_t addr, const char *space) const {
    if (space == nullptr || toupper(*space) == 'P')
        return raw_read(addr);
    if (toupper(*space) == 'M')
        return Regs.read_internal(addr);
    if (toupper(*space) == 'X')
        return DataMemory.read(addr);
    return 0;
}

void ProgI8051::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr || toupper(*space) == 'P') {
        raw_write(addr, data);
    } else if (toupper(*space) == 'M') {
        Regs.write_internal(addr, data);
    } else if (toupper(*space) == 'X') {
        DataMemory.write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *ProgI8051::assembler() const {
    static auto as = new libasm::i8051::AsmI8051();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *ProgI8051::disassembler() const {
    static auto dis = new libasm::i8051::DisI8051();

    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

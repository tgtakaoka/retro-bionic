#include "mems_i8048.h"
#include <asm_i8048.h>
#include <ctype.h>
#include <dis_i8048.h>
#include "devs_i8048.h"
#include "regs_i8048.h"

namespace debugger {
namespace i8048 {

struct ProgI8048 ProgMemory;
struct DataI8048 DataMemory;

uint16_t DataI8048::read(uint32_t addr) const {
    addr &= 0xFF;
    return Devs.isSelected(addr) ? Devs.read(addr) : raw_read(addr);
}

void DataI8048::write(uint32_t addr, uint16_t data) const {
    addr &= 0xFF;
    if (Devs.isSelected(addr)) {
        Devs.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t ProgI8048::get(uint32_t addr, const char *space) const {
    if (space == nullptr || toupper(*space) == 'P')
        return raw_read(addr);
    if (toupper(*space) == 'M')
        return Regs.read_internal(addr);
    if (toupper(*space) == 'X')
        return DataMemory.read(addr);
    return 0;
}

void ProgI8048::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr || toupper(*space) == 'P') {
        raw_write(addr, data);
    } else if (toupper(*space) == 'M') {
        Regs.write_internal(addr, data);
    } else if (toupper(*space) == 'X') {
        DataMemory.write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *ProgI8048::assembler() const {
    static auto as = new libasm::i8048::AsmI8048();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *ProgI8048::disassembler() const {
    static auto dis = new libasm::i8048::DisI8048();
    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace i8048
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

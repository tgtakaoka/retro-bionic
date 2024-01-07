#include <string.h>

#include <asm_z8.h>
#include <dis_z8.h>

#include "devs_z88.h"
#include "mems_z88.h"
#include "regs_z88.h"

namespace debugger {
namespace z88 {

struct MemsZ88 Memory;

namespace {
RegSpace parseSpace(const char *space) {
    if (strcasecmp_P(space, PSTR("set1")) == 0)
        return SET_ONE;
    if (strcasecmp_P(space, PSTR("set2")) == 0)
        return SET_TWO;
    if (strcasecmp_P(space, PSTR("bank1")) == 0)
        return SET_BANK1;
    return SET_NONE;
}
}  // namespace

uint16_t MemsZ88::read(uint32_t addr) const {
    return Devs.isSelected(addr) ? Devs.read(addr) : raw_read(addr);
}

void MemsZ88::write(uint32_t addr, uint16_t data) const {
    if (Devs.isSelected(addr)) {
        Devs.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsZ88::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read(addr);
    return Regs.read_reg(addr, parseSpace(space));
}

void MemsZ88::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write(addr, data);
    } else {
        // TODO: Write SET TWO registers and BANK1 registers
        Regs.write_reg(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsZ88::assembler() const {
    static auto as = new libasm::z8::AsmZ8();
    as->setCpu(Regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsZ88::disassembler() const {
    static auto dis = new libasm::z8::DisZ8();
    dis->setCpu(Regs.cpu());
    return dis;
}
#endif

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

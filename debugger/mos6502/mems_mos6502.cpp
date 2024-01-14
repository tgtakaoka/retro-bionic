#include <asm_mos6502.h>
#include <dis_mos6502.h>

#include "devs_mos6502.h"
#include "inst_mos6502.h"
#include "mems_mos6502.h"
#include "pins_mos6502.h"
#include "regs_mos6502.h"

namespace debugger {
namespace mos6502 {

struct MemsMos6502 Memory;

uint16_t MemsMos6502::read(uint32_t addr) const {
    return Devices.isSelected(addr) ? Devices.read(addr) : raw_read(addr);
}

void MemsMos6502::write(uint32_t addr, uint16_t data) const {
    if (Devices.isSelected(addr)) {
        Devices.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsMos6502::assembler() const {
    static auto as = new libasm::mos6502::AsmMos6502();
    as->setCpu(Registers.cpu());
    as->setOption("longa", "off");
    as->setOption("longi", "off");
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsMos6502::disassembler() const {
    static auto dis = new libasm::mos6502::DisMos6502();
    dis->setCpu(Registers.cpu());
    dis->setOption("longa", "off");
    dis->setOption("longi", "off");
    return dis;
}
#endif

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

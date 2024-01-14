#include <asm_mos6502.h>
#include <dis_mos6502.h>

#include "devs_mos6502.h"
#include "inst_mos6502.h"
#include "mems_w65c816.h"
#include "pins_mos6502.h"
#include "regs_mos6502.h"

namespace debugger {
namespace w65c816 {

using mos6502::Devices;
using mos6502::Registers;

struct MemsW65c816 Memory;

uint16_t MemsW65c816::read(uint32_t addr) const {
    return Devices.isSelected(addr) ? Devices.read(addr) : raw_read(addr);
}

void MemsW65c816::write(uint32_t addr, uint16_t data) const {
    if (Devices.isSelected(addr)) {
        Devices.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsW65c816::assembler() const {
    static auto as = new libasm::mos6502::AsmMos6502();
    as->setCpu(Registers.cpu());
    as->setOption("longa", _longA ? "on" : "off");
    as->setOption("longi", _longI ? "on" : "off");
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsW65c816::disassembler() const {
    static auto dis = new libasm::mos6502::DisMos6502();
    dis->setCpu(Registers.cpu());
    dis->setOption("longa", _longA ? "on" : "off");
    dis->setOption("longi", _longI ? "on" : "off");
    return dis;
}
#endif

}  // namespace w65c816
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

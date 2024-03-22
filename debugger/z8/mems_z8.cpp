#include "mems_z8.h"
#include <asm_z8.h>
#include <dis_z8.h>
#include "regs_z8.h"

namespace debugger {
namespace z8 {

uint16_t MemsZ8::read(uint32_t addr) const {
    return _devs.isSelected(addr) ? _devs.read(addr) : raw_read(addr);
}

void MemsZ8::write(uint32_t addr, uint16_t data) const {
    if (_devs.isSelected(addr)) {
        _devs.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsZ8::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read(addr);
    return _regs.read_reg(addr, _regs.parseSpace(space));
}

void MemsZ8::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write(addr, data);
    } else {
        _regs.write_reg(addr, data, _regs.parseSpace(space));
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsZ8::assembler() const {
    static auto as = new libasm::z8::AsmZ8();
    as->setCpu(_regs.cpu());
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsZ8::disassembler() const {
    static auto dis = new libasm::z8::DisZ8();
    dis->setCpu(_regs.cpu());
    return dis;
}
#endif

}  // namespace z8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

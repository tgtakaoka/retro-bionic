#include "mems_z8000.h"
#include <asm_z8000.h>
#include <dis_z8000.h>
#include "inst_z8000.h"
#include "regs_z8000.h"

namespace debugger {
namespace z8000 {

MemsZ8000::MemsZ8000(bool segmented)
    : ExtMemory(Endian::ENDIAN_BIG, true),
      _segmented(segmented),
      _maxAddr(segmented ? 0x7FFFFF : UINT16_MAX) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::z8000::AsmZ8000();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::z8000::DisZ8000();
#endif
}

bool MemsZ8000::nonsegmented() const {
    return _segmented && _regs && (_regs->fcw() & InstZ8000::FCW_SEG) == 0;
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsZ8000::assembler() const {
    auto as = ExtMemory::assembler();
    if (as && nonsegmented())
        as->setCpu("Z8002");
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsZ8000::disassembler() const {
    auto dis = ExtMemory::disassembler();
    if (dis && nonsegmented())
        dis->setCpu("Z8002");
    return dis;
}
#endif

}  // namespace z8000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

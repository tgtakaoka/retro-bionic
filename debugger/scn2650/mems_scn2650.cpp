#include "mems_scn2650.h"
#include <asm_scn2650.h>
#include <dis_scn2650.h>

namespace debugger {
namespace scn2650 {

MemsScn2650::MemsScn2650() : DmaMemory(Endian::ENDIAN_BIG) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::scn2650::AsmScn2650();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::scn2650::DisScn2650();
#endif
}  // namespace scn2650

}  // namespace scn2650
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#include "mems_ns32016.h"
#include <asm_ns32000.h>
#include <dis_ns32000.h>

namespace debugger {
namespace ns32016 {

// libasm's NS32032 is the same instruction set; the NS32081 board turns
// its floating point instructions on.
MemsNs32016::MemsNs32016(bool fpu) : ExtMemory(Endian::ENDIAN_LITTLE, true) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::ns32000::AsmNs32000();
    _assembler->setOption("fpu", fpu ? "ns32081" : "none");
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::ns32000::DisNs32000();
    _disassembler->setOption("fpu", fpu ? "ns32081" : "none");
#endif
}

}  // namespace ns32016
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

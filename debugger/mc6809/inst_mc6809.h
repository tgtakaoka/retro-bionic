#ifndef __INST_MC6809_H__
#define __INST_MC6809_H__

#include "mc6800/inst_mc6800.h"
#include "regs_mc6809.h"

namespace debugger {
namespace mc6809 {

using mc6800::InstMc6800;
using mc6809::SoftwareType;

struct InstMc6809 : InstMc6800 {
    InstMc6809(const DmaMemory *mems) : InstMc6800(mems) {}

    virtual void setSoftwareType(SoftwareType type) = 0;

    static constexpr uint8_t NOP = 0x12;
};

}  // namespace mc6809
}  // namespace debugger
#endif
// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

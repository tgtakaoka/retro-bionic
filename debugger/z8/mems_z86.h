#ifndef __MEMS_Z86_H__
#define __MEMS_Z86_H__

#include "mems_z8.h"

namespace debugger {
namespace z86 {

struct RegsZ86;

struct MemsZ86 final : z8::MemsZ8 {
    MemsZ86(RegsZ86 *regs, Devs *dev);

    RomArea *romArea() override { return &_rom; }

private:
    RomArea _rom;
};

}  // namespace z86
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

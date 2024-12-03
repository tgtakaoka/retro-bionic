#ifndef __MEMS_Z86_H__
#define __MEMS_Z86_H__

#include "mems_z8.h"

namespace debugger {
namespace z86 {

using z8::MemsZ8;

struct MemsZ86 final : MemsZ8 {
    MemsZ86();

    RomArea *romArea() override { return &_rom; }

private:
    RomArea _rom;
};

extern struct MemsZ86 Memory;

}  // namespace z86
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

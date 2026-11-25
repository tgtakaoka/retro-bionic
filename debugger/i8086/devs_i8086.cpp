#include "devs_i8086.h"
#include "cpm86_disk.h"

namespace debugger {

DevsI8086::DevsI8086(const Mems *mems) : DevsI8080(new Cpm86Disk(mems)) {}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

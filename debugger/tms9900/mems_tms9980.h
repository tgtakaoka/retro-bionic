#ifndef __MEMS_TMS9980_H__
#define __MEMS_TMS9980_H__

#include "mems_tms9900.h"

namespace debugger {
namespace tms9980 {

struct PinsTms9980;

struct MemsTms9980 final : tms9900::MemsTms9900 {
    MemsTms9980(Devs *devs) : MemsTms9900(14, devs) {}
};

}  // namespace tms9980
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

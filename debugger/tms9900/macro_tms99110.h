#ifndef __MACRO_TMS99110_H__
#define __MACRO_TMS99110_H__

#include "mems_tms99105.h"

namespace debugger {
namespace tms99110 {

struct MacroTms99110 final {
    static void load(tms99105::MemsTms99105 *mems);
};

}  // namespace tms99110
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

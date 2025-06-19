#ifndef __SIGNALS_P8095BH_H__
#define __SIGNALS_P8095BH_H__

#include "signals_i8096.h"

namespace debugger {
namespace p8095bh {
struct Signals final : SignalsBase<Signals, i8096::SignalsI8096> {
    bool getControl();
};
}  // namespace p8095bh
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __SIGNALS_N8097BH_H__
#define __SIGNALS_N8097BH_H__

#include "signals_i8096.h"

namespace debugger {
namespace n8097bh {
struct Signals final : SignalsBase<Signals, i8096::SignalsI8096> {
    bool getControl();
};
}  // namespace n8097bh
}  // namespace debugger
#endif /* __SIGNALS_N8097BH_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __SIGNALS_S80296SA_H__
#define __SIGNALS_S80296SA_H__

#include "signals_i8096.h"

namespace debugger {
namespace s80296sa {
struct Signals final : SignalsBase<Signals, i8096::SignalsI8096> {
    bool getControl();
};
}  // namespace s80296sa
}  // namespace debugger
#endif /* __SIGNALS_S80296SA_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

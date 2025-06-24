#ifndef __SIGNALS_TMS370C250_H__
#define __SIGNALS_TMS370C250_H__

#include "signals_tms370.h"

namespace debugger {
namespace tms370c250 {

struct SignalsTms370C250 final
    : SignalsBase<SignalsTms370C250, tms370::Signals> {
    void getAddr();
    bool getDirection();
    void getData();
    void outData() const;
    void inputMode() const;
};

}  // namespace tms370c250
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

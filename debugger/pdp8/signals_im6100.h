#ifndef __SIGNALS_IM6100_H__
#define __SIGNALS_IM6100_H__

#include "signals_pdp8.h"

namespace debugger {
namespace im6100 {

struct Signals final : SignalsBase<Signals, pdp8::Signals> {
    void getAddress();
    bool getControl();
    void getDirection();
    void getData();
    void outData() const;
    static void inputMode();
    void outIoc(uint8_t ioc);

    bool hasSelect() const;
};

}  // namespace im6100
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

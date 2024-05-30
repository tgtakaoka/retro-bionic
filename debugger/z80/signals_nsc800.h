#ifndef __SIGNALS_NSC800_H__
#define __SIGNALS_NSC800_H__

#include "signals_z80.h"

namespace debugger {
namespace nsc800 {

struct SignalsNsc800 final : SignalsBase<SignalsNsc800, z80::Signals> {
    void getAddress();
    void getControl();
    void getData();
    void outData() const;
    void inputMode() const;

    bool nobus() const;
    bool memory() const;
    bool intack() const;

private:
    uint8_t &nsc800() { return _signals[2]; }
    uint8_t nsc800() const { return _signals[2]; }
};

}  // namespace nsc800
}  // namespace debugger
#endif /* __SIGNALS_NSC800_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

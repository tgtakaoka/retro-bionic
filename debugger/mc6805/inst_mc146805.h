#ifndef __INST_MC146805_H__
#define __INST_MC146805_H__

#include "inst_mc6805.h"

namespace debugger {
namespace mc146805 {

struct InstMc146805 : mc6805::InstMc6805 {
    bool isStop(uint8_t inst) const override { return inst == STOP; }

    static constexpr auto RESET_VEC = UINT16_C(0x1FFE);

protected:
    const uint8_t *table() const override;

    static constexpr uint8_t STOP = 0x8E;
    static constexpr uint8_t WAIT = 0x8F;
};

}  // namespace mc146805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

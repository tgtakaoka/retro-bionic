#ifndef __INST_MC68HC05_H__
#define __INST_MC68HC05_H__

#include "inst_mc146805.h"

namespace debugger {
namespace mc68hc05 {

struct InstMc68HC05 : mc146805::InstMc146805 {

    static constexpr auto RESET_VEC = UINT16_C(0xFFFE);

protected:
    const uint8_t *table() const override;
};

}  // namespace mc68hc05
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

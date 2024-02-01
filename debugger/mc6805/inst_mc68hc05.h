#ifndef __INST_MC68HC05_H__
#define __INST_MC68HC05_H__

#include "inst_mc146805.h"

namespace debugger {
namespace mc68hc05 {

using mc146805::InstMc146805;

struct InstMc68hc05 final : InstMc146805 {
protected:
    const uint8_t *table() const override;

    static constexpr uint8_t MUL = 0x42;
};

extern struct InstMc146805 Inst;

}  // namespace mc146805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

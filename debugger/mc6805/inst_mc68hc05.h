#ifndef __INST_MC68HC05_H__
#define __INST_MC68HC05_H__

#include "inst_mc6805.h"

namespace debugger {
namespace mc68hc05 {

using mc6805::InstMc6805;

struct InstMc68HC05 final : InstMc6805 {
protected:
    const uint8_t *table() const override;
};

extern const struct InstMc68HC05 Inst;

}  // namespace mc68hc05
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

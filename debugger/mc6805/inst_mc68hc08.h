#ifndef __INST_MC68HC08_H__
#define __INST_MC68HC08_H__

#include "inst_mc68hc05.h"
#include "mems.h"
#include "signals_mc6805.h"

namespace debugger {
namespace mc68hc08 {

using mc6805::Signals;

struct InstMc68HC08 final : mc68hc05::InstMc68HC05 {
    InstMc68HC08(Mems *mems) : _mems(mems) {}

    const char *sequence(Signals *s) const;

private:
    Mems *_mems;
};

}  // namespace mc68hc08
}  // namespace debugger
#endif
// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

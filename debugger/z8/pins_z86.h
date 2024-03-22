#ifndef __PINS_Z86_H__
#define __PINS_Z86_H__

/**
 * External memory operation
 * R248:D7; %F8 Write only
 *   1 = P04~7=A12~15
 * R248:D43; %F8 Write only
 *   10 = P10~7=AD0~7
 * R248:D1; %F8 Write only
 *   1 = P00~3=A8~11
 *
 * Stack selection
 * R248:D2; %F8 Write only
 *   0 = External
 *   1 = Internal
 */

/**
 * Data memory operation
 * R247:D43; %F7 Write only
 *   00 = P33=INPUT, P34=OUTPUT
 *   01 = P33=INPUT, P34=#DM
 *   10 = P33=INPUT, P34=#DM
 *
 * Serial IO
 * R247:D6; %F7 Write only
 *   1 = P30=Serial In, P37=Serial Out
 */

#include "pins_z8.h"

namespace debugger {
namespace z86 {

using z8::PinsZ8;

struct PinsZ86 final : PinsZ8 {
    PinsZ86();

private:
    void xtal1_cycle() const override;
    bool fetchVectorAfterContextSave() override { return true; }
};

extern struct PinsZ86 Pins;

}  // namespace z86
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

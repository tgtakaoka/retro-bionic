#ifndef __PINS_Z88_H__
#define __PINS_Z88_H__

/**
 * BANK0:R240:%F0 Write only; [P0M] Port 0 Mode
 * D7-D0: Address or I/O
 *   0001_1111 = P07-P05, A12-A8
 */

/**
 * BANK0:R241:%F1 Write only; [PM] Port Mode
 * D5-4: Port 1 mode
 *   1x = Address/Data
 *   01 = Input
 *   00 = Output
 * D3: DM enable
 *   1 = Enable DM P35
 *   0 = Disable DM P35
 */

/**
 * BANK0:R254:%F8 Write only; [EMT] External Memory Timing
 * D7: Wait input selection
 *   1 = External #WAIT input
 *   0 = I/O
 * D6: Slow memory timing
 *   1 = Enabled
 *   0 = Disabled
 * D5-4: Program memory automatic waits
 *   0~3 = waits
 * D3-2: Data memory automatic waits
 *   0~3 = waits
 * D1: Stack location selection
 *   1 = Data memory
 *   0 = Register file
 * D0: DMA select
 *   1 = Data memory
 *   0 = Register file
 */

#define PIN_WAIT 32 /* P7.12 */

#include "pins_z8.h"

namespace debugger {
namespace z88 {

using z8::PinsZ8;

struct PinsZ88 final : PinsZ8 {
    PinsZ88();

private:
    void xtal1_cycle() const override;
    bool fetchVectorAfterContextSave() override { return false; }
};

extern struct PinsZ88 Pins;

}  // namespace z88
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

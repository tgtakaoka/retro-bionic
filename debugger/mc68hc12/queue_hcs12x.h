#ifndef __QUEUE_HCS12X_H__
#define __QUEUE_HCS12X_H__

#include "signals_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

/**
 * Rebuilds the HCS12X instruction queue from IQSTAT3:0. The status sampled
 * in a cycle's ECLK-low phase reports the execution start of the cycle
 * before it and the data movement of the one before that (S12XCPUV2
 * 8.3.2); moved back onto those cycles, it is what the HCS12 shows, and
 * QueueHcs12 replays it.
 */
struct QueueHcs12x {
    void replay(Signals *begin, uint_fast8_t cycles);

    // ATTENTION: How many cycles back the start and the ALD of a sample
    // refer to; S12XCPUV2 Figure 8-3 and S12XEBIV2 Table 21-9 disagree by
    // one, unbenched.
    static constexpr uint8_t START_LAG = 1;
    static constexpr uint8_t ALD_LAG = 2;

    // IQSTAT3:0
    static constexpr uint8_t IQ_INT = 0x1;
    static constexpr uint8_t IQ_SOD = 0x2;
    static constexpr uint8_t IQ_SEV = 0x4;
    static constexpr uint8_t IQ_ALD = 0x8;
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

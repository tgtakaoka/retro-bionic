#ifndef __QUEUE_HCS12_H__
#define __QUEUE_HCS12_H__

#include "signals_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

/**
 * Rebuilds the HCS12 instruction queue from IPIPE1:0 over the captured
 * cycles, as S12CPUV2 8.4 describes: three stages and no holding latch.
 * The execution start, sampled before the E rise, refers to this cycle and
 * the opcode at the head of stage 3; the movement, sampled before the E
 * fall, is always ALD and takes this cycle's word into stage 1.
 */
struct QueueHcs12 {
    void reset();
    // Replays |cycles| cycles from |begin|, marking them.
    void replay(Signals *begin, uint_fast8_t cycles);

private:
    struct Word {
        uint16_t addr;
        uint16_t data;
        bool valid;
    };
    Word _st1;
    Word _st2;
    Word _st3;
    // The last start, and whether its opcode was the page 2 prebyte: it
    // counts as a one-byte instruction of its own.
    uint16_t _start;
    bool _prebyte;
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

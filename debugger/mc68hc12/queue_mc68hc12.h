#ifndef __QUEUE_MC68HC12_H__
#define __QUEUE_MC68HC12_H__

#include "signals_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

/**
 * Rebuilds the M68HC12 instruction queue from IPIPE1:0 over the captured
 * cycles, as CPU12RM 8.8 describes: two queue stages and a holding latch.
 * Each instruction start (SEV/SOD) marks the cycle the instruction begins
 * with its address; each program word the queue takes marks its cycle.
 */
struct Queue {
    void reset();
    // Replays |cycles| cycles from |begin|, marking them.
    void replay(Signals *begin, uint_fast8_t cycles);

private:
    struct Word {
        uint16_t addr;
        bool valid;
    };
    Word _st1;
    Word _st2;
    Word _latch;  // the "fetch" buffer; valid is CPU12RM's is_full
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

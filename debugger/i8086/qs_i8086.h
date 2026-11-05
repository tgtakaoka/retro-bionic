#ifndef __QS_I8086_H__
#define __QS_I8086_H__

#include <stdint.h>

namespace debugger {
namespace i8086 {

// The queue status, QS1 and QS0, sampled once per CLK period into a ring.
struct QsTracker {
    // Where a board has the queue status and what takes the samples.
    enum Board : uint8_t {
        // QS0 on P34 (P7.16), QS1 on P42 (P9.06); DMA at each rising edge
        // of CLK, FlexPWM3's.
        P8086,
        // The board's clock loop, through push().
        N80C186,
        // No queue status: nothing samples, and nothing freezes.
        N80C286,
    };
    QsTracker(Board board = P8086) : _board(board) {}
    void begin();
    // The sample, in bus clocks after CLK's rising edge.
    static uint16_t delay();
    // At a stop the ring stops too, so the prompt's idle clocks keep off
    // it; the next run lets it go on. |last| is when the latest sample was
    // taken, in CPU cycles.
    void freeze(uint32_t last);
    void thaw();
    // When the latest sample was taken, in CPU cycles, and the queue status
    // |back| samples before it.
    uint32_t stopCycle() const { return _stopCycle; }
    bool frozen() const { return _frozen; }
    uint8_t sampleAt(uint32_t back) const;
    // N80C186: the sample of the CLKOUT period just clocked.
    void push(uint8_t qs) {
        if (!_frozen)
            _samples[_pushed++ % SAMPLES] = qs;
    }

    static constexpr uint32_t SAMPLES = 4096;

private:
    const Board _board;
    bool _started = false;
    bool _frozen = false;
    uint32_t _stopIndex = 0;
    uint32_t _stopCycle = 0;
    uint32_t _pushed = 0;
    uint8_t _samples[SAMPLES];
};

}  // namespace i8086
}  // namespace debugger
#endif /* __QS_I8086_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

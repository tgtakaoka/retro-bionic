#ifndef __QS_I8086_H__
#define __QS_I8086_H__

#include <stdint.h>

namespace debugger {
namespace i8086 {

// The queue status, QS1 and QS0, sampled once per CLK period into a ring
// by DMA, off the CPU.
struct QsTracker {
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

    static constexpr uint32_t SAMPLES = 4096;

private:
    bool _started = false;
    bool _frozen = false;
    uint32_t _stopIndex = 0;
    uint32_t _stopCycle = 0;
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

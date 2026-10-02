#ifndef __DEBUGGER_WATCHDOG_H__
#define __DEBUGGER_WATCHDOG_H__

#include <stdint.h>

namespace debugger {

/**
 * RTWDOG reboots the Teensy when the firmware stops making progress: the
 * prompt, a completed bus cycle and a halt switch poll feed it, so only
 * a loop that does none of them, which nothing could stop, trips it.
 */
struct Watchdog {
    static void begin();
    static void feed();
    /**
     * Feeds every 65536 calls, as the counter wraps, cheap enough for every
     * bus cycle; a caller must tick at least every 61 us to beat the 4 s
     * timeout.
     */
    static void tick() {
        if (++_ticks == 0)
            feed();
    }
    /** Whether the last reboot was the watchdog's; clears the flag. */
    static bool fired();

private:
    static uint16_t _ticks;
};

}  // namespace debugger
#endif /* __DEBUGGER_WATCHDOG_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

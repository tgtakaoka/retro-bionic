#include "watchdog.h"

#include <Arduino.h>

namespace debugger {

namespace {
// RTWDOG counts the 32 kHz LPO clock divided by 256: 500 is 4 s.
constexpr uint16_t TIMEOUT = 500;
}  // namespace

uint16_t Watchdog::_ticks;

// RTWDOG, for its refresh: one 32-bit write once CMD32EN is set.
void Watchdog::begin() {
    CCM_CCGR5 |= CCM_CCGR5_WDOG3(CCM_CCGR_ON);
    __disable_irq();
    if (WDOG3_CS & WDOG_CS_CMD32EN) {
        WDOG3_CNT = 0xD928C520;  // unlock
    } else {
        *(volatile uint16_t *)&WDOG3_CNT = 0xC520;
        *(volatile uint16_t *)&WDOG3_CNT = 0xD928;
    }
    WDOG3_WIN = 0;
    WDOG3_TOVAL = TIMEOUT;
    WDOG3_CS = WDOG_CS_CMD32EN | WDOG_CS_PRES | WDOG_CS_CLK(1) | WDOG_CS_FLG |
               WDOG_CS_UPDATE | WDOG_CS_EN;
    __enable_irq();
}

void Watchdog::feed() {
    WDOG3_CNT = 0xB480A602;
}

bool Watchdog::fired() {
    if ((SRC_SRSR & SRC_SRSR_WDOG3_RST_B) == 0)
        return false;
    SRC_SRSR = SRC_SRSR_WDOG3_RST_B;  // write 1 to clear
    return true;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

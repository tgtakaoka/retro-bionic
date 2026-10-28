#ifdef FLEXIO_PROBE

#include "flexio_probe.h"
#include <stdlib.h>
#include "debugger.h"
#include "digital_bus.h"
#include "pwm_clock.h"
#include "watchdog.h"

namespace debugger {
namespace {

// P10-P27: GPIO6.16-31, pads GPIO_AD_B1_00-15, FlexIO3 pins 0-15.
#define PORT_AD 6
#define AD_gp 16
#define AD_gm 0xFFFF
#define AD_vp 0
volatile uint32_t *const AD_MUX = &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_00;

// The emulated CPU: ALE from FlexPWM3_1_A on P47, a strobe from
// FlexPWM3_1_B on P53, one period for both. On the mezzanine, P47 goes to
// P34 (FlexIO3 pin 16) and P53 to the select of a 157, whose 1Y is #RD on
// P35 and 2Y #WR on P36: with 1I1 and 2I1 on VCC, a strobe reaches #RD
// when 1I0 (P54) is low and #WR when 2I0 (P55) is low.
constexpr uint8_t PIN_ALE_OUT = 29; /* P9.31 P47 */
constexpr uint8_t PIN_STB_OUT = 28; /* P8.18 P53 */
constexpr uint8_t PIN_ALE_IN = 8;   /* P7.16 P34 */
constexpr uint8_t PIN_RD = 7;       /* P7.17 P35 */
constexpr uint8_t PIN_WR = 36;      /* P7.18 P36 */
constexpr uint8_t PIN_RD_EN = 31;   /* P8.22 P54 */
constexpr uint8_t PIN_WR_EN = 30;   /* P8.23 P55 */
constexpr uint8_t FLEX_ALE = 16;
constexpr uint8_t FLEX_RD = 17;
constexpr uint8_t FLEX_WR = 18;
// The CPU's wait input, driven by FlexIO3 itself.
constexpr uint8_t PIN_WAIT = 37; /* P7.19 P37 */
constexpr uint8_t FLEX_WAIT = 19;

constexpr uint32_t MUX_GPIO = 5 | 0x10;  // ALT5 + SION, as pinMode()

struct Config {
    uint32_t shiftctl;
    uint32_t shiftcfg;
    uint32_t timctl;
    uint32_t timcfg;
    uint32_t timcmp;
    uint32_t muxFlexio;
    uint8_t pred;
    uint8_t podf;
};

// Receive shifter 3, the first that can sample pins in parallel, takes all
// 16 pins on the rising edge of the shift clock. Timer 0, in 8-bit baud
// mode like a UART receiver, starts as ALE falls (as #AS rises when ALE_LOW
// drops TIMCTL's PINPOL), raises the shift clock one FlexIO clock later and
// stores SHIFTBUF3 at the next. A shift clock on the ALE pin itself cannot
// latch at the fall: the store comes before the shift of the same edge.
constexpr auto SH_ALE = 3;
constexpr uint32_t ALE_HIGH = FLEXIO_TIMCTL_PINSEL(FLEX_ALE) |
                              FLEXIO_TIMCTL_PINPOL | FLEXIO_TIMCTL_TIMOD(1);
constexpr uint32_t ALE_LOW = ALE_HIGH & ~FLEXIO_TIMCTL_PINPOL;
// Start on a pin rise, shift clock low first, stop at the compare.
constexpr uint32_t LATCH_TIMCFG = FLEXIO_TIMCFG_TIMOUT(1) |
                                  FLEXIO_TIMCFG_TIMDIS(2) |
                                  FLEXIO_TIMCFG_TIMENA(4);
// One shift: (2 edges - 1) << 8, each edge a FlexIO clock apart.
constexpr uint32_t LATCH_TIMCMP = 0x100;

Config config = {
        FLEXIO_SHIFTCTL_TIMSEL(0) | FLEXIO_SHIFTCTL_PINSEL(0) |
                FLEXIO_SHIFTCTL_SMOD(1),
        FLEXIO_SHIFTCFG_PWIDTH(15),
        ALE_HIGH,
        LATCH_TIMCFG,
        LATCH_TIMCMP,
        9 | 0x10,  // ALT9 + SION, as FlexIO_t4
        1,         // PLL3 480 MHz / 2 / 2 = 120 MHz
        1,
};

// Full-FlexIO cycles. Read data: transmit shifter 0 holds the word on
// P10-P27 as bidirectional output data, and transmit shifter 4 is its
// output enable, all ones while #RD is low and its zero stop bit after.
// Timer 3 runs both from #RD on P35: enabled on its fall, disabled, with a
// stop bit, on its rise; its shift clock is the trigger, so the stop state
// ends at the next fall. Write data: receive shifter 7 takes P10-P27 just
// after #WR on P36 rises, timer 2 latching as timer 0 does.
// #WAIT on P37: timer 1 drives it from the moment timer 0 starts latching
// an address until the firmware writes SHIFTBUF1. Transmit shifter 1, run
// by timer 1, reloads as #WAIT asserts, setting its flag; the write clears
// the flag, and that falling trigger ends timer 1, releasing #WAIT.
constexpr auto SH_DATA = 0;
constexpr auto SH_WAIT = 1;
constexpr auto SH_OE = 4;
constexpr auto SH_WR = 7;
constexpr uint32_t TRG_RD = 4 * (FLEX_RD / 2) + 2 * (FLEX_RD % 2);
constexpr uint32_t TRG_SH_WAIT = 4 * SH_WAIT + 1;

struct Reg {
    const char *name;
    volatile uint32_t *reg;
    uint32_t val;
};

Reg full[] = {
        {"sf0", &FLEXIO3_SHIFTCFG0 + SH_DATA, FLEXIO_SHIFTCFG_PWIDTH(15)},
        {"sc0", &FLEXIO3_SHIFTCTL0 + SH_DATA,
                FLEXIO_SHIFTCTL_TIMSEL(3) | FLEXIO_SHIFTCTL_TIMPOL |
                        FLEXIO_SHIFTCTL_PINCFG(2) | FLEXIO_SHIFTCTL_PINSEL(0) |
                        FLEXIO_SHIFTCTL_SMOD(2)},
        {"sf4", &FLEXIO3_SHIFTCFG0 + SH_OE,
                FLEXIO_SHIFTCFG_PWIDTH(15) | FLEXIO_SHIFTCFG_SSTOP(2)},
        {"sc4", &FLEXIO3_SHIFTCTL0 + SH_OE,
                FLEXIO_SHIFTCTL_TIMSEL(3) | FLEXIO_SHIFTCTL_TIMPOL |
                        FLEXIO_SHIFTCTL_PINCFG(1) | FLEXIO_SHIFTCTL_PINSEL(0) |
                        FLEXIO_SHIFTCTL_SMOD(2)},
        {"sf7", &FLEXIO3_SHIFTCFG0 + SH_WR, FLEXIO_SHIFTCFG_PWIDTH(15)},
        {"sc7", &FLEXIO3_SHIFTCTL0 + SH_WR,
                FLEXIO_SHIFTCTL_TIMSEL(2) | FLEXIO_SHIFTCTL_PINSEL(0) |
                        FLEXIO_SHIFTCTL_SMOD(1)},
        {"sf1", &FLEXIO3_SHIFTCFG0 + SH_WAIT, 0},
        {"sc1", &FLEXIO3_SHIFTCTL0 + SH_WAIT,
                FLEXIO_SHIFTCTL_TIMSEL(1) | FLEXIO_SHIFTCTL_SMOD(2)},
        {"tf3", &FLEXIO3_TIMCFG3,
                FLEXIO_TIMCFG_TIMDEC(3) | FLEXIO_TIMCFG_TIMDIS(6) |
                        FLEXIO_TIMCFG_TIMENA(6) | FLEXIO_TIMCFG_TSTOP(2)},
        {"tm3", &FLEXIO3_TIMCMP3, 0xFFFF},
        {"tc3", &FLEXIO3_TIMCTL3,
                FLEXIO_TIMCTL_TRGSEL(TRG_RD) | FLEXIO_TIMCTL_TRGPOL |
                        FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TIMOD(3)},
        // Output one, so #WAIT low, while enabled; enabled with timer 0,
        // disabled as shifter 1's flag falls. PINPOL off makes it WAIT.
        {"tf1", &FLEXIO3_TIMCFG1,
                FLEXIO_TIMCFG_TIMDIS(6) | FLEXIO_TIMCFG_TIMENA(1)},
        {"tm1", &FLEXIO3_TIMCMP1, 0xFFFF},
        {"tc1", &FLEXIO3_TIMCTL1,
                FLEXIO_TIMCTL_TRGSEL(TRG_SH_WAIT) | FLEXIO_TIMCTL_TRGSRC |
                        FLEXIO_TIMCTL_PINCFG(3) |
                        FLEXIO_TIMCTL_PINSEL(FLEX_WAIT) |
                        FLEXIO_TIMCTL_PINPOL | FLEXIO_TIMCTL_TIMOD(3)},
        {"tf2", &FLEXIO3_TIMCFG2, LATCH_TIMCFG},
        {"tm2", &FLEXIO3_TIMCMP2, LATCH_TIMCMP},
        {"tc2", &FLEXIO3_TIMCTL2,
                FLEXIO_TIMCTL_PINSEL(FLEX_WR) | FLEXIO_TIMCTL_TIMOD(1)},
};

uint32_t toNs(uint32_t cycles) {
    return uint64_t(cycles) * 1000 / (F_CPU_ACTUAL / 1000000);
}

uint32_t countToNs(uint32_t counts) {
    return uint64_t(counts) * 1000000000 / F_BUS_ACTUAL;
}

uint16_t nsToCount(uint32_t ns) {
    return uint64_t(ns) * F_BUS_ACTUAL / 1000000000;
}

struct Stat {
    uint32_t min, max, sum, n;
    void clear() { min = UINT32_MAX, max = sum = n = 0; }
    void add(uint32_t c) {
        if (c < min)
            min = c;
        if (c > max)
            max = c;
        sum += c;
        ++n;
    }
    void print(const char *name, uint32_t (*ns)(uint32_t)) const {
        cli.print(name);
        if (n == 0) {
            cli.println(" -");
            return;
        }
        cli.print(" min ");
        cli.printDec(ns(min));
        cli.print(" avg ");
        cli.printDec(ns(sum / n));
        cli.print(" max ");
        cli.printDec(ns(max));
        cli.println(" ns");
    }
};

void setupFlexio() {
    CCM_CCGR7 &= ~CCM_CCGR7_FLEXIO3(CCM_CCGR_ON);
    CCM_CSCMR2 = (CCM_CSCMR2 & ~CCM_CSCMR2_FLEXIO2_CLK_SEL(3)) |
                 CCM_CSCMR2_FLEXIO2_CLK_SEL(3);
    CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_FLEXIO2_CLK_PRED(7) |
                                        CCM_CS1CDR_FLEXIO2_CLK_PODF(7))) |
                 CCM_CS1CDR_FLEXIO2_CLK_PRED(config.pred) |
                 CCM_CS1CDR_FLEXIO2_CLK_PODF(config.podf);
    CCM_CCGR7 |= CCM_CCGR7_FLEXIO3(CCM_CCGR_ON);
    FLEXIO3_CTRL = FLEXIO_CTRL_SWRST;
    FLEXIO3_CTRL = 0;
    // The reset ends in FlexIO's clock domain; a write right after it was
    // lost in about a third of the runs.
    delayNanoseconds(100);
    (&FLEXIO3_SHIFTCFG0)[SH_ALE] = config.shiftcfg;
    (&FLEXIO3_SHIFTCTL0)[SH_ALE] = config.shiftctl;
    FLEXIO3_TIMCMP0 = config.timcmp;
    FLEXIO3_TIMCFG0 = config.timcfg;
    FLEXIO3_TIMCTL0 = config.timctl;
    FLEXIO3_CTRL = FLEXIO_CTRL_FLEXEN;
}

// Shifters before timers, each configuration before its mode, as the
// table runs; shifter 4 then starts on its stop bit, outputs disabled.
void setupFull() {
    for (const auto &r : full)
        *r.reg = r.val;
}

void adToFlexio() {
    for (auto i = 0; i < 16; ++i)
        AD_MUX[i] = config.muxFlexio;
}

void adToGpio() {
    for (auto i = 0; i < 16; ++i)
        AD_MUX[i] = MUX_GPIO;
}

// Hold |val| on P10-P27: drive it, then let the base board's bus holders
// keep it.
void holdAd(uint16_t val) {
    busWriteFast(AD, val);
    busMode(AD, OUTPUT);
    delayNanoseconds(20);
    busMode(AD, INPUT);
}

uint16_t nextPattern(uint32_t &x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x & 0xFFFF;
}

// FlexIO3 owns P34 (and P35-P37 for full cycles) and, between cycles,
// P10-P27.
constexpr uint8_t FLEX_INS[] = {PIN_ALE_IN, PIN_RD, PIN_WR, PIN_WAIT};
uint32_t inMux[4];
uint8_t flexIns;

void beginFlexio(bool fullCycles = false) {
    flexIns = fullCycles ? 4 : 1;
    for (auto i = 0; i < flexIns; ++i) {
        inMux[i] = *portConfigRegister(FLEX_INS[i]);
        pinMode(FLEX_INS[i], INPUT);
        *portConfigRegister(FLEX_INS[i]) = config.muxFlexio;
    }
    setupFlexio();
    if (fullCycles)
        setupFull();
    (void)(&FLEXIO3_SHIFTBUF0)[SH_ALE];
}

void endFlexio() {
    FLEXIO3_CTRL = 0;
    adToGpio();
    busMode(AD, INPUT);
    for (auto i = 0; i < flexIns; ++i)
        *portConfigRegister(FLEX_INS[i]) = inMux[i];
}

// The emulated CPU's ALE (A) and strobe (B), as function statics so that
// nothing runs before the command.
PwmClock<PIN_ALE_OUT> &aleClock() {
    static PwmClock<PIN_ALE_OUT> clock(0, 1000);
    return clock;
}
PwmClock<PIN_STB_OUT> &strobeClock() {
    static PwmClock<PIN_STB_OUT> clock(0, 1000);
    return clock;
}
uint32_t pwmHz;
uint16_t aleOn, aleOff, stbOn, stbOff;

void stopPwm() {
    if (pwmHz == 0)
        return;
    aleClock().stop(pwmHz);
    strobeClock().stop(pwmHz);
    pinMode(PIN_ALE_OUT, INPUT);
    pinMode(PIN_STB_OUT, INPUT);
    pwmHz = 0;
}

// ALE high from |aOn| to |aOff| ns of a period of |hz|; the strobe high
// from |bOn| to |bOff| ns, which wraps when |bOn| > |bOff|, so an active-low
// strobe low from t1 to t2 is "t2 t1".
void startPwm(uint32_t hz, uint32_t aOn, uint32_t aOff, uint32_t bOn,
        uint32_t bOff) {
    stopPwm();
    auto &ale = aleClock();
    auto &stb = strobeClock();
    ale.begin(hz);
    stb.begin(hz);
    aleOn = nsToCount(aOn);
    aleOff = nsToCount(aOff);
    stbOn = nsToCount(bOn);
    stbOff = nsToCount(bOff);
    ale.setEdges(aleOn, aleOff);
    stb.setEdges(stbOn, stbOff);
    ale.start();
    stb.start();
    pwmHz = hz;
}

void printPwm() {
    if (pwmHz == 0) {
        cli.println("PWM off");
        return;
    }
    const auto period = PwmClock<PIN_ALE_OUT>::period();
    cli.print("PWM ");
    cli.printDec(F_BUS_ACTUAL / period);
    cli.print(" Hz, period ");
    cli.printDec(countToNs(period));
    cli.print(" ns; ALE ");
    cli.printDec(countToNs(aleOn));
    cli.print("-");
    cli.printDec(countToNs(aleOff));
    cli.print(" strobe ");
    cli.printDec(countToNs(stbOn));
    cli.print("-");
    cli.printDec(countToNs(stbOff));
    cli.println(" ns");
}

// One latch per pattern, ALE pulsed from P47 as a GPIO: hold the pattern,
// hand the pads to FlexIO3, pulse ALE, wait for SHIFTSTAT, read SHIFTBUF,
// take the pads back, and drive and read a data word.
void latch(uint32_t count, uint32_t aleNs) {
    stopPwm();
    Stat toFlexio, flag, buf, toGpio, data;
    toFlexio.clear();
    flag.clear();
    buf.clear();
    toGpio.clear();
    data.clear();
    uint32_t held = 0, early = 0, timeout = 0, hi = 0, lo = 0, dataOk = 0;
    uint32_t sion = 0, lastBuf = 0, lastPattern = 0;

    digitalWriteFast(PIN_ALE_OUT, LOW);
    pinMode(PIN_ALE_OUT, OUTPUT);
    beginFlexio();

    uint32_t x = 0x2545F491;
    for (uint32_t i = 0; i < count; ++i) {
        const auto pattern = nextPattern(x);
        noInterrupts();
        holdAd(pattern);
        delayNanoseconds(50);
        if (busRead(AD) == pattern)
            ++held;

        auto t0 = ARM_DWT_CYCCNT;
        adToFlexio();
        auto t1 = ARM_DWT_CYCCNT;
        toFlexio.add(t1 - t0);
        sion |= busRead(AD);

        digitalWriteFast(PIN_ALE_OUT, HIGH);
        delayNanoseconds(aleNs);
        if (FLEXIO3_SHIFTSTAT & (1 << SH_ALE))
            ++early;
        digitalWriteFast(PIN_ALE_OUT, LOW);
        t0 = ARM_DWT_CYCCNT;
        bool seen;
        while (!(seen = FLEXIO3_SHIFTSTAT & (1 << SH_ALE)) &&
                ARM_DWT_CYCCNT - t0 < F_CPU_ACTUAL / 100000)
            ;
        t1 = ARM_DWT_CYCCNT;
        const auto v = (&FLEXIO3_SHIFTBUF0)[SH_ALE];
        const auto t2 = ARM_DWT_CYCCNT;
        if (seen) {
            flag.add(t1 - t0);
            buf.add(t2 - t1);
        } else {
            ++timeout;
        }

        t0 = ARM_DWT_CYCCNT;
        adToGpio();
        t1 = ARM_DWT_CYCCNT;
        toGpio.add(t1 - t0);

        const uint16_t word = pattern ^ 0xFFFF;
        t0 = ARM_DWT_CYCCNT;
        busWriteFast(AD, word);
        busMode(AD, OUTPUT);
        const auto back = busRead(AD);
        busMode(AD, INPUT);
        t1 = ARM_DWT_CYCCNT;
        interrupts();
        Watchdog::tick();
        data.add(t1 - t0);
        if (back == word)
            ++dataOk;
        if ((v >> 16) == pattern)
            ++hi;
        if ((v & 0xFFFF) == pattern)
            ++lo;
        lastBuf = v;
        lastPattern = pattern;
    }

    endFlexio();
    pinMode(PIN_ALE_OUT, INPUT);

    cli.print("latches ");
    cli.printDec(count);
    cli.print(" ALE ");
    cli.printDec(aleNs);
    cli.println(" ns");
    cli.print("held ");
    cli.printDec(held);
    cli.print(" early ");
    cli.printDec(early);
    cli.print(" timeout ");
    cli.printDec(timeout);
    cli.print(" hi ");
    cli.printDec(hi);
    cli.print(" lo ");
    cli.printDec(lo);
    cli.print(" data ");
    cli.printDec(dataOk);
    cli.print(" gpio-in-flexio ");
    cli.printlnHex(sion, 4);
    cli.print("last pattern ");
    cli.printHex(lastPattern, 4);
    cli.print(" SHIFTBUF ");
    cli.printlnHex(lastBuf, 8);
    toFlexio.print("pads to FlexIO  ", toNs);
    flag.print("ALE fall to flag", toNs);
    buf.print("SHIFTBUF read   ", toNs);
    toGpio.print("pads to GPIO    ", toNs);
    data.print("data write+read ", toNs);
}

// Wait, at most 10 us, for the bits |mask| of |pins| to read |level|.
bool waitPin(volatile uint32_t &pins, uint32_t mask, bool level) {
    const auto t0 = ARM_DWT_CYCCNT;
    while (((pins & mask) != 0) != level) {
        if (ARM_DWT_CYCCNT - t0 >= F_CPU_ACTUAL / 100000)
            return false;
    }
    return true;
}

// Wait, at most 10 us, for shifter |sh|'s status flag.
bool waitFlag(uint32_t sh) {
    const auto t0 = ARM_DWT_CYCCNT;
    while (!(FLEXIO3_SHIFTSTAT & (1 << sh))) {
        if (ARM_DWT_CYCCNT - t0 >= F_CPU_ACTUAL / 100000)
            return false;
    }
    return true;
}

// Start serving right after a latch, so that none is pending with a stale
// address: drop it, then hold |addr| for the next.
void syncStart(uint16_t addr) {
    adToFlexio();
    (void)waitFlag(SH_ALE);
    (void)(&FLEXIO3_SHIFTBUF0)[SH_ALE];
    adToGpio();
    holdAd(addr);
    adToFlexio();
}

// Let the next strobe through as #WR or #RD.
void selectCycle(bool write) {
    digitalWriteFast(PIN_RD_EN, write);
    digitalWriteFast(PIN_WR_EN, !write);
}

void beginSelect(bool write) {
    pinMode(PIN_RD, INPUT);
    pinMode(PIN_WR, INPUT);
    selectCycle(write);
    pinMode(PIN_RD_EN, OUTPUT);
    pinMode(PIN_WR_EN, OUTPUT);
}

void endSelect() {
    pinMode(PIN_RD_EN, INPUT);
    pinMode(PIN_WR_EN, INPUT);
}

// Whether cycle |i| of |mode| writes: r reads, w writes, x alternates,
// v picks at random.
bool isWrite(char mode, uint32_t i, uint32_t &x) {
    if (mode == 'w')
        return true;
    if (mode == 'x')
        return i & 1;
    if (mode == 'v')
        return nextPattern(x) & 1;
    return false;
}

// Serve |count| bus cycles of the running PWM, as the debugger would on a
// multiplexed bus: wait for the latch flag, take the pads, then on #RD
// drive a data word until #RD rises, or on #WR read the bus as #WR rises;
// then pick the next cycle's strobe, hold the next address and hand the
// pads back before the next ALE. Each event is stamped with the PWM count,
// the place in the period.
void serve(uint32_t count, char mode) {
    if (pwmHz == 0) {
        cli.println("?PWM off");
        return;
    }
    uint32_t reads = 0, writes = 0;
    uint32_t y = 0x9E3779B9;
    auto write = isWrite(mode, 0, y);
    beginSelect(write);
    Stat flag, padsGpio, low, served, high, back;
    flag.clear();
    padsGpio.clear();
    low.clear();
    served.clear();
    high.clear();
    back.clear();
    uint32_t match = 0, waiting = 0, timeout = 0, lastBuf = 0;
    uint16_t lastAddr = 0;

    beginFlexio();
    uint32_t x = 0x2545F491;
    auto addr = nextPattern(x);
    noInterrupts();
    syncStart(addr);
    for (uint32_t i = 0; i < count; ++i) {
        if (FLEXIO3_SHIFTSTAT & (1 << SH_ALE))
            ++waiting;
        const auto t0 = ARM_DWT_CYCCNT;
        bool seen;
        while (!(seen = FLEXIO3_SHIFTSTAT & (1 << SH_ALE)) &&
                ARM_DWT_CYCCNT - t0 < F_CPU_ACTUAL / 100000)
            ;
        if (!seen) {
            ++timeout;
            break;
        }
        flag.add(PwmClock<PIN_ALE_OUT>::count());
        const auto v = (&FLEXIO3_SHIFTBUF0)[SH_ALE];
        adToGpio();
        padsGpio.add(PwmClock<PIN_ALE_OUT>::count());
        const uint32_t strobe = write ? CORE_PIN36_BITMASK : CORE_PIN7_BITMASK;
        if (!waitPin(GPIO7_PSR, strobe, LOW)) {
            ++timeout;
            break;
        }
        low.add(PwmClock<PIN_ALE_OUT>::count());
        if (!write) {
            busWriteFast(AD, addr ^ 0xFFFF);
            busMode(AD, OUTPUT);
            served.add(PwmClock<PIN_ALE_OUT>::count());
        }
        if (!waitPin(GPIO7_PSR, strobe, HIGH)) {
            ++timeout;
            break;
        }
        if (write) {
            (void)busRead(AD);
            ++writes;
        } else {
            ++reads;
        }
        high.add(PwmClock<PIN_ALE_OUT>::count());
        write = isWrite(mode, i + 1, y);
        selectCycle(write);
        if ((v >> 16) == addr)
            ++match;
        lastBuf = v;
        lastAddr = addr;
        addr = nextPattern(x);
        holdAd(addr);
        adToFlexio();
        back.add(PwmClock<PIN_ALE_OUT>::count());
        Watchdog::tick();
    }
    interrupts();
    endFlexio();
    endSelect();

    printPwm();
    cli.print("reads ");
    cli.printDec(reads);
    cli.print(" writes ");
    cli.printDec(writes);
    cli.print(" match ");
    cli.printDec(match);
    cli.print(" flag-waiting ");
    cli.printDec(waiting);
    cli.print(" timeout ");
    cli.printDec(timeout);
    cli.print(" last address ");
    cli.printHex(lastAddr, 4);
    cli.print(" SHIFTBUF ");
    cli.printlnHex(lastBuf, 8);
    cli.println("place in the period:");
    flag.print("flag seen      ", countToNs);
    padsGpio.print("pads to GPIO   ", countToNs);
    low.print("strobe low seen", countToNs);
    served.print("data driven    ", countToNs);
    high.print("strobe high    ", countToNs);
    back.print("pads to FlexIO ", countToNs);
}

// Serve |count| bus cycles with FlexIO3 owning P10-P27 through each cycle:
// the address from shifter 3; read data staged in shifters 0 and 4 as soon
// as the address is known, for FlexIO to drive while #RD is low; write data
// from shifter 7 after #WR rises; FLEXIO3_PIN times the events. As the
// emulated CPU, the firmware takes the pads back to hold the write data
// after ALE and the next address after the strobe.
void serveFull(uint32_t count, char mode) {
    if (pwmHz == 0) {
        cli.println("?PWM off");
        return;
    }
    uint32_t reads = 0, writes = 0, match = 0, drove = 0, released = 0;
    uint32_t captured = 0, early = 0, timeout = 0, errors = 0;
    uint32_t lastBuf = 0, lastData = 0;
    uint16_t lastAddr = 0;
    uint32_t y = 0x9E3779B9;
    auto write = isWrite(mode, 0, y);
    beginSelect(write);
    Stat flag, staged, low, high, stored, back;
    flag.clear();
    staged.clear();
    low.clear();
    high.clear();
    stored.clear();
    back.clear();

    beginFlexio(true);
    uint32_t x = 0x2545F491;
    auto addr = nextPattern(x);
    noInterrupts();
    syncStart(addr);
    (void)(&FLEXIO3_SHIFTBUF0)[SH_WR];
    FLEXIO3_SHIFTERR = 0xFF;
    for (uint32_t i = 0; i < count; ++i) {
        if (!waitFlag(SH_ALE)) {
            ++timeout;
            break;
        }
        flag.add(PwmClock<PIN_ALE_OUT>::count());
        const auto v = (&FLEXIO3_SHIFTBUF0)[SH_ALE];
        if ((v >> 16) == addr)
            ++match;
        const uint32_t strobe = 1 << (write ? FLEX_WR : FLEX_RD);
        uint16_t data;
        if (write) {
            data = addr ^ 0x5A5A;
            adToGpio();
            holdAd(data);
            adToFlexio();
        } else {
            data = addr ^ 0xFFFF;
            (&FLEXIO3_SHIFTBUF0)[SH_OE] = UINT32_MAX;
            (&FLEXIO3_SHIFTBUF0)[SH_DATA] = data | (uint32_t(data) << 16);
        }
        (&FLEXIO3_SHIFTBUF0)[SH_WAIT] = 0;  // release #WAIT
        staged.add(PwmClock<PIN_ALE_OUT>::count());
        if (!waitPin(FLEXIO3_PIN, strobe, LOW)) {
            ++timeout;
            break;
        }
        low.add(PwmClock<PIN_ALE_OUT>::count());
        if (write) {
            if (FLEXIO3_SHIFTSTAT & (1 << SH_WR))
                ++early;
        } else {
            delayNanoseconds(30);
            if (uint16_t(FLEXIO3_PIN) == data)
                ++drove;
        }
        if (!waitPin(FLEXIO3_PIN, strobe, HIGH)) {
            ++timeout;
            break;
        }
        high.add(PwmClock<PIN_ALE_OUT>::count());
        if (write) {
            if (!waitFlag(SH_WR)) {
                ++timeout;
                break;
            }
            stored.add(PwmClock<PIN_ALE_OUT>::count());
            lastBuf = (&FLEXIO3_SHIFTBUF0)[SH_WR];
            if ((lastBuf >> 16) == data)
                ++captured;
            ++writes;
        } else {
            ++reads;
        }
        errors |= FLEXIO3_SHIFTERR;
        FLEXIO3_SHIFTERR = 0xFF;
        write = isWrite(mode, i + 1, y);
        selectCycle(write);
        lastAddr = addr;
        lastData = data;
        addr = nextPattern(x);
        adToGpio();
        holdAd(addr);
        adToFlexio();
        delayNanoseconds(20);
        if (uint16_t(FLEXIO3_PIN) == addr)
            ++released;
        back.add(PwmClock<PIN_ALE_OUT>::count());
        Watchdog::tick();
    }
    interrupts();
    endFlexio();
    endSelect();

    printPwm();
    cli.print("reads ");
    cli.printDec(reads);
    cli.print(" drove ");
    cli.printDec(drove);
    cli.print(" writes ");
    cli.printDec(writes);
    cli.print(" captured ");
    cli.printDec(captured);
    cli.print(" early ");
    cli.printDec(early);
    cli.println();
    cli.print("match ");
    cli.printDec(match);
    cli.print(" released ");
    cli.printDec(released);
    cli.print(" timeout ");
    cli.printDec(timeout);
    cli.print(" SHIFTERR ");
    cli.printlnHex(errors, 2);
    cli.print("last address ");
    cli.printHex(lastAddr, 4);
    cli.print(" data ");
    cli.printHex(lastData, 4);
    cli.print(" SHIFTBUF7 ");
    cli.printlnHex(lastBuf, 8);
    cli.println("place in the period:");
    flag.print("address latched", countToNs);
    staged.print("data staged    ", countToNs);
    low.print("strobe low seen", countToNs);
    high.print("strobe high    ", countToNs);
    stored.print("write stored   ", countToNs);
    back.print("next address   ", countToNs);
}

// Serve |count| cycles with FlexIO3 alone, as fast as the bus goes: the
// emulated CPU never drives P10-P27, so each address is whatever the bus
// holders kept, the last read data. Per cycle the firmware reads the latch,
// stages read data from |memory|, as prepareCycle would, and only then does
// the bookkeeping and the emulation, steering the next strobe after this one. Every latch
// must equal the last data driven, and every write capture the held value.
uint16_t memory[4096];

// Whether #WAIT on P37 is asserted, by timer 1's pin polarity.
bool waitAsserted() {
    const bool high = FLEXIO3_PIN & (1 << FLEX_WAIT);
    return (FLEXIO3_TIMCTL1 & FLEXIO_TIMCTL_PINPOL) ? !high : high;
}
// Whether each read refills the output-enable shifter, or leaves it to
// reload its stale all-ones buffer.
bool refillOe = true;
// Whether c h times each cycle's software and stamps one cycle in 64 with
// the place in the period, at about 150 ns for that cycle.
bool stamping = false;

void serveHeld(uint32_t count, char mode) {
    if (pwmHz == 0) {
        cli.println("?PWM off");
        return;
    }
    uint32_t reads = 0, writes = 0, match = 0, captured = 0, behind = 0;
    uint32_t timeout = 0, errors = 0, lastBuf = 0, overrunBlocks = 0;
    uint32_t waitAtLatch = 0, stuck = 0;
    uint32_t y = 0x9E3779B9;
    auto write = isWrite(mode, 0, y);
    beginSelect(write);
    Stat staged, prepare, loop, released;
    staged.clear();
    prepare.clear();
    loop.clear();
    released.clear();
    uint32_t x = 0x2545F491;
    for (auto &m : memory)
        m = nextPattern(x);

    beginFlexio(true);
    noInterrupts();
    adToFlexio();
    (&FLEXIO3_SHIFTBUF0)[SH_OE] = UINT32_MAX;
    (void)waitFlag(SH_ALE);
    (void)(&FLEXIO3_SHIFTBUF0)[SH_ALE];
    (void)(&FLEXIO3_SHIFTBUF0)[SH_WR];
    FLEXIO3_SHIFTERR = 0xFF;
    bool known = false;
    uint16_t held = 0, writeHeld = 0;
    uint32_t badLatch = 0, badCapture = 0;
    constexpr uint32_t ALE_FLAG = 1 << SH_ALE;
    constexpr uint32_t WR_FLAG = 1 << SH_WR;
    for (uint32_t i = 0; i < count; ++i) {
        // One SHIFTSTAT read answers both the latch and a pending write.
        auto stat = FLEXIO3_SHIFTSTAT;
        if (stat & ALE_FLAG) {
            ++behind;
        } else {
            const auto start = ARM_DWT_CYCCNT;
            do {
                stat = FLEXIO3_SHIFTSTAT;
            } while (!(stat & ALE_FLAG) &&
                     ARM_DWT_CYCCNT - start < F_CPU_ACTUAL / 100000);
            if (!(stat & ALE_FLAG)) {
                ++timeout;
                break;
            }
        }
        const auto t0 = ARM_DWT_CYCCNT;
        if (stamping && waitAsserted())
            ++waitAtLatch;
        const uint16_t addr = (&FLEXIO3_SHIFTBUF0)[SH_ALE] >> 16;
        uint16_t data = addr;
        if (!write) {
            data = memory[addr & 0xFFF];
            (&FLEXIO3_SHIFTBUF0)[SH_DATA] = data | (uint32_t(data) << 16);
            if (refillOe)
                (&FLEXIO3_SHIFTBUF0)[SH_OE] = UINT32_MAX;
        }
        const auto t1 = ARM_DWT_CYCCNT;
        (&FLEXIO3_SHIFTBUF0)[SH_WAIT] = 0;  // release #WAIT
        if (stamping) {
            const auto r0 = ARM_DWT_CYCCNT;
            while (waitAsserted() && ARM_DWT_CYCCNT - r0 < F_CPU_ACTUAL / 1000000)
                ;
            if (waitAsserted()) {
                ++stuck;
            } else {
                released.add(ARM_DWT_CYCCNT - r0);
            }
        }
        // Only mixed cycles steer the strobe; the change must come after
        // the last strobe and before the next, so it limits their speed.
        if (mode == 'x' || mode == 'v')
            selectCycle(write);
        // A write is picked up at a later poll, off the critical path.
        if (stat & WR_FLAG) {
            lastBuf = (&FLEXIO3_SHIFTBUF0)[SH_WR];
            if ((lastBuf >> 16) == writeHeld) {
                ++captured;
            } else if (badCapture == 0) {
                badCapture = uint32_t(writeHeld) << 16 | (lastBuf >> 16);
            }
        }
        if (known && addr == held) {
            ++match;
        } else if (known && badLatch == 0) {
            badLatch = uint32_t(held) << 16 | addr;
        }
        held = data;
        known = true;
        if (write) {
            writeHeld = addr;
            ++writes;
        } else {
            ++reads;
        }
        write = isWrite(mode, i + 1, y);
        if (stamping) {
            prepare.add(t1 - t0);
            loop.add(ARM_DWT_CYCCNT - t0);
            if ((i & 63) == 0)
                staged.add(PwmClock<PIN_ALE_OUT>::count());
        }
        // Overruns are sticky; check them, and the watchdog, once a block.
        if ((i & 1023) == 1023) {
            const auto err = FLEXIO3_SHIFTERR;
            if (err & (ALE_FLAG | WR_FLAG))
                ++overrunBlocks;
            errors |= err;
            FLEXIO3_SHIFTERR = 0xFF;
            Watchdog::tick();
        }
    }
    // The last write's capture may still be pending.
    if (waitFlag(SH_WR)) {
        lastBuf = (&FLEXIO3_SHIFTBUF0)[SH_WR];
        if ((lastBuf >> 16) == writeHeld)
            ++captured;
    }
    interrupts();
    endFlexio();
    endSelect();

    printPwm();
    cli.print("reads ");
    cli.printDec(reads);
    cli.print(" writes ");
    cli.printDec(writes);
    cli.print(" match ");
    cli.printDec(match);
    cli.print(" captured ");
    cli.printDec(captured);
    cli.print(" behind ");
    cli.printDec(behind);
    cli.print(" timeout ");
    cli.printDec(timeout);
    cli.print(" SHIFTERR ");
    cli.printlnHex(errors, 2);
    cli.print("1024-cycle blocks with an overrun ");
    cli.printlnDec(overrunBlocks);
    cli.print("first bad latch expected:got ");
    cli.printHex(badLatch, 8);
    cli.print(" capture ");
    cli.printlnHex(badCapture, 8);
    if (stamping) {
        staged.print("data staged, place in the period", countToNs);
        prepare.print("latch read to data staged", toNs);
        loop.print("latch read to loop end   ", toNs);
        cli.print("#WAIT asserted at latch read ");
        cli.printDec(waitAtLatch);
        cli.print(" stuck ");
        cli.printlnDec(stuck);
        released.print("release write to #WAIT off", toNs);
    }
}

// Cost of single accesses, 100 each.
void timing() {
    CCM_CCGR7 |= CCM_CCGR7_FLEXIO3(CCM_CCGR_ON);
    constexpr auto N = 100;
    noInterrupts();
    auto t0 = ARM_DWT_CYCCNT;
    for (auto i = 0; i < N; ++i)
        (void)FLEXIO3_SHIFTSTAT;
    auto t1 = ARM_DWT_CYCCNT;
    const auto shiftstat = t1 - t0;
    t0 = ARM_DWT_CYCCNT;
    for (auto i = 0; i < N; ++i)
        (void)FLEXIO3_PIN;
    t1 = ARM_DWT_CYCCNT;
    const auto pin = t1 - t0;
    t0 = ARM_DWT_CYCCNT;
    for (auto i = 0; i < N; ++i)
        (void)GPIO6_PSR;
    t1 = ARM_DWT_CYCCNT;
    const auto psr = t1 - t0;
    t0 = ARM_DWT_CYCCNT;
    for (auto i = 0; i < N; ++i)
        AD_MUX[i & 15] = MUX_GPIO;
    t1 = ARM_DWT_CYCCNT;
    const auto mux = t1 - t0;
    t0 = ARM_DWT_CYCCNT;
    for (auto i = 0; i < N; ++i)
        (void)PwmClock<PIN_ALE_OUT>::count();
    t1 = ARM_DWT_CYCCNT;
    const auto cnt = t1 - t0;
    interrupts();
    cli.print("ns per 100: SHIFTSTAT ");
    cli.printDec(toNs(shiftstat));
    cli.print(" FlexIO PIN ");
    cli.printDec(toNs(pin));
    cli.print(" GPIO6 PSR ");
    cli.printDec(toNs(psr));
    cli.print(" pad mux write ");
    cli.printDec(toNs(mux));
    cli.print(" PWM CNT ");
    cli.printlnDec(toNs(cnt));
}

void printConfig() {
    cli.print("sc ");
    cli.printHex(config.shiftctl, 8);
    cli.print(" sf ");
    cli.printHex(config.shiftcfg, 8);
    cli.print(" tc ");
    cli.printHex(config.timctl, 8);
    cli.print(" tf ");
    cli.printHex(config.timcfg, 8);
    cli.print(" tm ");
    cli.printHex(config.timcmp, 8);
    cli.print(" mux ");
    cli.printHex(config.muxFlexio, 2);
    cli.print(" clk ");
    cli.printDec(480 / (config.pred + 1) / (config.podf + 1));
    cli.println(" MHz");
    for (auto i = 0u; i < sizeof(full) / sizeof(full[0]); ++i) {
        cli.print(full[i].name);
        cli.print(' ');
        cli.printHex(full[i].val, 8);
        cli.print(i % 6 == 5 ? "\r\n" : " ");
    }
}

bool setConfig(const char *name, uint32_t val) {
    for (auto &r : full) {
        if (strcmp(name, r.name) == 0) {
            r.val = val;
            return true;
        }
    }
    if (strcmp(name, "sc") == 0) {
        config.shiftctl = val;
    } else if (strcmp(name, "sf") == 0) {
        config.shiftcfg = val;
    } else if (strcmp(name, "tc") == 0) {
        config.timctl = val;
    } else if (strcmp(name, "tf") == 0) {
        config.timcfg = val;
    } else if (strcmp(name, "tm") == 0) {
        config.timcmp = val;
    } else if (strcmp(name, "mux") == 0) {
        config.muxFlexio = val;
    } else if (strcmp(name, "pred") == 0 && val < 8) {
        config.pred = val;
    } else if (strcmp(name, "podf") == 0 && val < 8) {
        config.podf = val;
    } else {
        return false;
    }
    return true;
}

const char *skipSpaces(const char *p) {
    while (*p == ' ')
        ++p;
    return p;
}

// Up to |max| numbers from |p|, in |base|; returns how many.
int numbers(const char *p, uint32_t *vals, int max, int base) {
    auto n = 0;
    while (n < max) {
        p = skipSpaces(p);
        if (*p == 0)
            break;
        char *end;
        vals[n++] = strtoul(p, &end, base);
        if (end == p)
            break;
        p = end;
    }
    return n;
}

constexpr char USAGE[] =
        "l [n [ns]]            : latch n patterns, ALE pulsed n ns from P47\r\n"
        "a hz aOn aOff bOn bOff : PWM ALE on P47, strobe on P53, edges in ns\r\n"
        "a 0                   : PWM off\r\n"
        "c r|w|x|v [n]         : serve n cycles: reads (#RD on P35), writes\r\n"
        "                        (#WR on P36), alternating, or random; P54\r\n"
        "                        and P55 steer the strobe\r\n"
        "c f r|w|x|v [n]       : the same, FlexIO3 serving whole cycles\r\n"
        "c h r|w|x|v [n]       : FlexIO3 alone, the bus holding read data as\r\n"
        "                        the next address; #WAIT on P37 from ALE\r\n"
        "                        until the data is staged\r\n"
        "i 0|1                 : c h times its loop, stamps one cycle in 64\r\n"
        "o 0|1                 : c h refills the output enable each read\r\n"
        "p h|l                 : latch on ALE falling, or #AS rising\r\n"
        "t                     : time single accesses\r\n"
        "f                     : show the FlexIO3 setup\r\n"
        "s name hex            : set sc sf tc tf tm mux pred podf, or\r\n"
        "                        sf0 sc0 sf4 sc4 sf7 sc7 sf1 sc1 tf3 tm3 tc3\r\n"
        "                        tf1 tm1 tc1 tf2 tm2 tc2\r\n"
        "m pin [hex]           : show, or set, a pin's pad mux\r\n"
        "x addr [hex]          : read, or write, a 32-bit register\r\n";

}  // namespace

void flexioProbe(const char *line) {
    const auto cmd = *line;
    if (cmd)
        ++line;
    uint32_t v[5];
    // 'x' takes hex; 'm' a decimal pin, then hex.
    auto n = numbers(line, v, cmd == 'm' ? 1 : 5, cmd == 'x' ? 16 : 10);
    if (cmd == 'm' && n == 1) {
        line = skipSpaces(line);
        while (*line && *line != ' ')
            ++line;
        n += numbers(line, v + 1, 1, 16);
    }
    if (cmd == 'l') {
        latch(n > 0 ? v[0] : 1000, n > 1 ? v[1] : 100);
    } else if (cmd == 'a') {
        if (n == 0 || v[0] == 0) {
            stopPwm();
        } else if (n == 5) {
            startPwm(v[0], v[1], v[2], v[3], v[4]);
        } else {
            cli.println("?a hz aOn aOff bOn bOff");
        }
        printPwm();
    } else if (cmd == 'c') {
        line = skipSpaces(line);
        auto mode = *line;
        const auto fullCycles = mode == 'f';
        const auto heldCycles = mode == 'h';
        if (fullCycles || heldCycles) {
            line = skipSpaces(line + 1);
            mode = *line;
        }
        if (mode)
            ++line;
        const auto m = numbers(line, v, 1, 10);
        if (heldCycles) {
            serveHeld(m > 0 ? v[0] : 1000, mode);
        } else if (fullCycles) {
            serveFull(m > 0 ? v[0] : 1000, mode);
        } else {
            serve(m > 0 ? v[0] : 1000, mode);
        }
    } else if (cmd == 'i') {
        if (n > 0)
            stamping = v[0];
        cli.print("stamping ");
        cli.printlnDec(stamping);
    } else if (cmd == 'o') {
        if (n > 0)
            refillOe = v[0];
        cli.print("refill OE ");
        cli.printlnDec(refillOe);
    } else if (cmd == 'p') {
        line = skipSpaces(line);
        if (*line == 'h' || *line == 'l')
            config.timctl = *line == 'h' ? ALE_HIGH : ALE_LOW;
        printConfig();
    } else if (cmd == 't') {
        timing();
    } else if (cmd == 'f') {
        printConfig();
    } else if (cmd == 's') {
        line = skipSpaces(line);
        char name[8];
        auto i = 0;
        while (*line && *line != ' ' && i < 7)
            name[i++] = *line++;
        name[i] = 0;
        uint32_t val;
        if (numbers(line, &val, 1, 16) != 1 || !setConfig(name, val))
            cli.println("?s name hex");
        printConfig();
    } else if (cmd == 'm' && n >= 1 && v[0] < CORE_NUM_DIGITAL) {
        auto &mux = *portConfigRegister(v[0]);
        if (n == 2)
            mux = v[1];
        cli.print("pin ");
        cli.printDec(v[0]);
        cli.print(" mux ");
        cli.printlnHex(mux, 2);
    } else if (cmd == 'x' && n >= 1) {
        auto &reg = *reinterpret_cast<volatile uint32_t *>(v[0]);
        if (n == 2)
            reg = v[1];
        cli.printHex(v[0], 8);
        cli.print(" = ");
        cli.printlnHex(reg, 8);
    } else {
        cli.print(USAGE);
    }
}

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

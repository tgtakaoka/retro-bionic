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
// P34 (FlexIO3 pin 16) and P53 to #RD on P41 or #WR on P42.
constexpr uint8_t PIN_ALE_OUT = 29; /* P9.31 P47 */
constexpr uint8_t PIN_STB_OUT = 28; /* P8.18 P53 */
constexpr uint8_t PIN_ALE_IN = 8;   /* P7.16 P34 */
constexpr uint8_t PIN_RD = 3;       /* P9.05 P41 */
constexpr uint8_t PIN_WR = 4;       /* P9.06 P42 */
// Picks which strobe a cycle gets: the select of the mezzanine's 157s,
// whose Y0 goes to #RD and Y1 to #WR, the strobe on I0 of Y0 and I1 of Y1,
// VCC on the others; low makes a read cycle, high a write.
constexpr uint8_t PIN_SELECT = 31; /* P8.22 P54 */
constexpr uint8_t FLEX_ALE = 16;

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

// Receive shifter 0 takes all 16 pins on the falling edge of the shift
// clock; timer 0 makes the shift clock the ALE pin itself, and expires,
// storing SHIFTBUF0, every second edge.
Config config = {
        FLEXIO_SHIFTCTL_TIMSEL(0) | FLEXIO_SHIFTCTL_TIMPOL |
                FLEXIO_SHIFTCTL_PINSEL(0) | FLEXIO_SHIFTCTL_SMOD(1),
        FLEXIO_SHIFTCFG_PWIDTH(15),
        FLEXIO_TIMCTL_PINSEL(FLEX_ALE) | FLEXIO_TIMCTL_TIMOD(3),
        FLEXIO_TIMCFG_TIMDEC(2),
        1,
        9 | 0x10,  // ALT9 + SION, as FlexIO_t4
        1,         // PLL3 480 MHz / 2 / 2 = 120 MHz
        1,
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
    FLEXIO3_SHIFTCFG0 = config.shiftcfg;
    FLEXIO3_SHIFTCTL0 = config.shiftctl;
    FLEXIO3_TIMCMP0 = config.timcmp;
    FLEXIO3_TIMCFG0 = config.timcfg;
    FLEXIO3_TIMCTL0 = config.timctl;
    FLEXIO3_CTRL = FLEXIO_CTRL_FLEXEN;
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

// FlexIO3 owns P34 and, between cycles, P10-P27.
uint32_t aleInMux;

void beginFlexio() {
    aleInMux = *portConfigRegister(PIN_ALE_IN);
    pinMode(PIN_ALE_IN, INPUT);
    *portConfigRegister(PIN_ALE_IN) = config.muxFlexio;
    setupFlexio();
    (void)FLEXIO3_SHIFTBUF0;
}

void endFlexio() {
    FLEXIO3_CTRL = 0;
    adToGpio();
    busMode(AD, INPUT);
    *portConfigRegister(PIN_ALE_IN) = aleInMux;
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
        if (FLEXIO3_SHIFTSTAT & 1)
            ++early;
        digitalWriteFast(PIN_ALE_OUT, LOW);
        t0 = ARM_DWT_CYCCNT;
        bool seen;
        while (!(seen = FLEXIO3_SHIFTSTAT & 1) &&
                ARM_DWT_CYCCNT - t0 < F_CPU_ACTUAL / 100000)
            ;
        t1 = ARM_DWT_CYCCNT;
        const auto v = FLEXIO3_SHIFTBUF0;
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
    cli.print(" SHIFTBUF0 ");
    cli.printlnHex(lastBuf, 8);
    toFlexio.print("pads to FlexIO  ", toNs);
    flag.print("ALE fall to flag", toNs);
    buf.print("SHIFTBUF read   ", toNs);
    toGpio.print("pads to GPIO    ", toNs);
    data.print("data write+read ", toNs);
}

// Wait, at most 10 us, for the GPIO9 bits |mask| to read |level|.
bool waitPin(uint32_t mask, bool level) {
    const auto t0 = ARM_DWT_CYCCNT;
    while (((GPIO9_PSR & mask) != 0) != level) {
        if (ARM_DWT_CYCCNT - t0 >= F_CPU_ACTUAL / 100000)
            return false;
    }
    return true;
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
    digitalWriteFast(PIN_SELECT, write);
    pinMode(PIN_SELECT, OUTPUT);
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
    holdAd(addr);
    adToFlexio();
    (void)FLEXIO3_SHIFTBUF0;
    for (uint32_t i = 0; i < count; ++i) {
        if (FLEXIO3_SHIFTSTAT & 1)
            ++waiting;
        const auto t0 = ARM_DWT_CYCCNT;
        bool seen;
        while (!(seen = FLEXIO3_SHIFTSTAT & 1) &&
                ARM_DWT_CYCCNT - t0 < F_CPU_ACTUAL / 100000)
            ;
        if (!seen) {
            ++timeout;
            break;
        }
        flag.add(PwmClock<PIN_ALE_OUT>::count());
        const auto v = FLEXIO3_SHIFTBUF0;
        adToGpio();
        padsGpio.add(PwmClock<PIN_ALE_OUT>::count());
        const uint32_t strobe = write ? CORE_PIN4_BITMASK : CORE_PIN3_BITMASK;
        if (!waitPin(strobe, LOW)) {
            ++timeout;
            break;
        }
        low.add(PwmClock<PIN_ALE_OUT>::count());
        if (!write) {
            busWriteFast(AD, addr ^ 0xFFFF);
            busMode(AD, OUTPUT);
            served.add(PwmClock<PIN_ALE_OUT>::count());
        }
        if (!waitPin(strobe, HIGH)) {
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
        digitalWriteFast(PIN_SELECT, write);
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
    pinMode(PIN_SELECT, INPUT);

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
    cli.print(" SHIFTBUF0 ");
    cli.printlnHex(lastBuf, 8);
    cli.println("place in the period:");
    flag.print("flag seen      ", countToNs);
    padsGpio.print("pads to GPIO   ", countToNs);
    low.print("strobe low seen", countToNs);
    served.print("data driven    ", countToNs);
    high.print("strobe high    ", countToNs);
    back.print("pads to FlexIO ", countToNs);
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
}

bool setConfig(const char *name, uint32_t val) {
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
        "c r|w|x|v [n]         : serve n cycles: reads (#RD on P41), writes\r\n"
        "                        (#WR on P42), alternating, or random; the\r\n"
        "                        157 select on P54 picks the strobe\r\n"
        "t                     : time single accesses\r\n"
        "f                     : show the FlexIO3 setup\r\n"
        "s name hex            : set sc sf tc tf tm mux pred podf\r\n"
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
        const auto mode = *line;
        if (mode)
            ++line;
        const auto m = numbers(line, v, 1, 10);
        serve(m > 0 ? v[0] : 1000, mode);
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

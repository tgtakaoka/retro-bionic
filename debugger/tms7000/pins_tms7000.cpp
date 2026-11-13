#include "pins_tms7000.h"
#include <type_traits>
#include "debugger.h"
#include "devs_tms7000.h"
#include "inst_tms7000.h"
#include "mems_tms7000.h"
#include "regs_tms7000.h"
#include "signals_tms7000.h"

namespace debugger {
namespace tms7000 {

// clang-format off
/**
 * TMS7000NL2/TMS7000NL4/TMS70C00NL
 * TMS7001NL2/TMS7001NL4 bus cycle
 *              __    __    __    __    __    __    __    __    __    __    __
 *   /4 CLK |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *              _____       _____       _____       _____       _____       _____
 *   /2 CLK ___|     |_____|     |_____|     |_____|     |_____|     |_____|
 *          ___       _____       _____       _____       _____       _____
 * CMOS CLK    |_____|     |_____|     |_____|     |_____|     |_____|     |_____
 *             \ ____\_____\     \     \ ____\_____\           \ ____\_____\
 *  CLKOUT _____|     |     |_____|_____|     |     |___________|     |     |____
 *              v     v___________v     v     v     v           v     v__________
 *  ALATCH ___________|///////////|___________________________________|//////////
 *          ____v_______________________v                       v________________
 *  #ENABLE ____|                       |_______________________|
 *          __________v_______________________________________________v__________
 *     R/#W __________|                                               |__________
 *                    v______________             __v___________v     v__________
 *       AD ----------<______AL______>-----------<______________>-----<____AL____
 *          __________v                                               v__________
 *     R/#W __________|_______________________________________________|__________
 *                    v______________          _____v___________v     v__________
 *       AD ----------<______AL______>--------<_________________>-----<____AL____
 *
 * TMS7002NL/TMS70C02NL bus cycle
 *              _____       _____       _____       _____       _____       _____
 *   /2 CLK ___|     |_____|     |_____|     |_____|     |_____|     |_____|
 *          ___       _____       _____       _____       _____       _____
 * CMOS CLK    |_____|     |_____|     |_____|     |_____|     |_____|     |_____
 *             \ ____\_____\           \ __________\     \     \ ____\_____\
 *  CLKOUT _____|     |     |___________|           |_____v_____|     |     |____
 *              v     V_____v                             v           v_____v
 * TMS7002  __________|/////|_________________________________________|/////|____
 *          ____v_______________________v                 v______________________
 *  #ENABLE ____|                       |_________________|
 *
 */
// clang-format on

namespace {

// fOSC: min 1.0 MHz, max 5.0 MHz
//       max 10.1 MHz /4 TMS7001, 8.0 MHz TMS7002
//       max 5.0 MHz TMS70C00A, 6.0 MHz TMS70C02 at 5 V
//   tW: min  90 ns ; CLKIN high/low width
//   tC: min 400 ns ; CLKOUT cycle time
// tSUA: min  50 ns ; address valid setup for ALATCH-
//  tHA: min  30 ns ; address hold after ALATCH-
// tSUW: min  50 ns ; R/#W setup for ALATCH-
// tdEL: min -10 ns ; CLKOUT+ to #ENABLE-
//  tAD: min 155 ns ; data input valid for #ENABLE+

constexpr auto clkin_hi_ns = 500;
constexpr auto clkin_lo_ns = 500;

// The bus-cycle delays in ns.
// for CLK_DIV2, CLK_CMOS and TMS7002
enum Div2Delay : uint8_t {
    cycle_delay,
    div2_hi_ns,
    div2_lo_ns,
    div2_hi_alatch,
    div2_lo_alatch,
    div2_lo_addr,
    div2_lo_dir,
    div2_hi_write,
    div2_hi_internal,
    tms7002_hi_alatch,
    tms7002_lo_alatch,
    tms7002_hi_addr,
    // Data held after CLKIN falls, until the CPU takes it at #ENABLE rise.
    div2_lo_read,
    // CLKIN low after a write, before the next cycle clocks it high.
    div2_lo_write,
    DIV2_DELAYS,
};
// CLKIN within the NMOS /2 parts' external-clock limits: tc(P) 200 ns,
// tw(PH) and tw(PL) 90 ns. The TMS7002's phases set them on the bench; the
// TMS7001's own, div2_hi_alatch, div2_lo_alatch and div2_lo_addr, follow
// their TMS7002 counterparts. div2_lo_dir reads #ENABLE after it falls,
// 40-48 ns after CLKIN; div2_lo_read holds read data past its rise, 32-40
// ns after CLKIN.
constexpr uint16_t DIV2_DEFAULTS[] = {
        0,   // cycle_delay
        // CLKIN+ to CLKOUT+: typ 132 ns
        // CLKIN+ to CLKOUT-: typ  96 ns
        85,  // div2_hi_ns         100
        75,  // div2_lo_ns         100
        90,  // div2_hi_alatch     100
        80,  // div2_lo_alatch     100
        70,  // div2_lo_addr       100
        80,  // div2_lo_dir        100
        70,  // div2_hi_write      100
        70,  // div2_hi_internal   100
        // for TMS7002
        90,  // tms7002_hi_alatch  100
        70,  // tms7002_lo_alatch  100
        70,  // tms7002_hi_addr    100
        60,  // div2_lo_read
        25,  // div2_lo_write
};

// The TMS7002, rated 8 MHz: tc(P) 125 ns, tw(PH) and tw(PL) 50 ns.
constexpr uint16_t TMS7002_DEFAULTS[] = {
        0,   // cycle_delay
        48,  // div2_hi_ns
        58,  // div2_lo_ns
        40,  // div2_hi_alatch
        74,  // div2_lo_alatch
        30,  // div2_lo_addr
        20,  // div2_lo_dir
        40,  // div2_hi_write
        20,  // div2_hi_internal
        68,  // tms7002_hi_alatch
        70,  // tms7002_lo_alatch
        30,  // tms7002_hi_addr
        0,   // div2_lo_read
        0,   // div2_lo_write
};

// TMS70C00 and TMS70C02, which both take the TMS7002 timing: no CLKIN
// faster than the TMS70C00A's 5 MHz at 5 V +-10%.
constexpr uint16_t CMOS7002_DEFAULTS[] = {
        0,   // cycle_delay
        72,  // div2_hi_ns
        87,  // div2_lo_ns
        60,  // div2_hi_alatch
        111,  // div2_lo_alatch
        45,  // div2_lo_addr
        30,  // div2_lo_dir
        72,  // div2_hi_write
        30,  // div2_hi_internal
        102,  // tms7002_hi_alatch
        105,  // tms7002_lo_alatch
        45,  // tms7002_hi_addr
        0,   // div2_lo_read
        0,   // div2_lo_write
};
// 6 MHz, the TMS70C02's and the wide-voltage TMS70C00A's at 5.0 V.
// constexpr uint16_t CMOS7002_DEFAULTS[] = {
//         0,   // cycle_delay
//         57,  // div2_hi_ns
//         69,  // div2_lo_ns
//         48,  // div2_hi_alatch
//         88,  // div2_lo_alatch
//         36,  // div2_lo_addr
//         24,  // div2_lo_dir
//         0,   // div2_hi_write
//         24,  // div2_hi_internal
//         81,  // tms7002_hi_alatch
//         84,  // tms7002_lo_alatch
//         36,  // tms7002_hi_addr
//         0,   // div2_lo_read
//         0,   // div2_lo_write
// };
// 8 MHz: passes the regress on the bench, past every CMOS datasheet.
// constexpr uint16_t CMOS7002_DEFAULTS[] = {
//         0,   // cycle_delay
//         36,  // div2_hi_ns
//         43,  // div2_lo_ns
//         30,  // div2_hi_alatch
//         55,  // div2_lo_alatch
//         22,  // div2_lo_addr
//         15,  // div2_lo_dir
//         0,   // div2_hi_write
//         15,  // div2_hi_internal
//         51,  // tms7002_hi_alatch
//         52,  // tms7002_lo_alatch
//         22,  // tms7002_hi_addr
//         0,   // div2_lo_read
//         0,   // div2_lo_write
// };

// for CLK_DIV4
enum Div4Delay : uint8_t {
    div4_cycle_delay,
    div4_lo_ns,
    div4_hi_serial,
    div4_hi_wait,
    div4_lo_alatch,
    div4_lo_latched,
    div4_lo_addr,
    div4_lo_inject,
    div4_hi_read,
    div4_hi_internal,
    div4_hi_capture,
    DIV4_DELAYS,
};
constexpr uint16_t DIV4_DEFAULTS[] = {
        0,   // div4_cycle_delay
        38,  // div4_lo_ns         50
        20,  // div4_hi_serial     50
        20,  // div4_hi_wait       50
        10,  // div4_lo_alatch     50
        20,  // div4_lo_latched    50
        4,   // div4_lo_addr       50
        10,  // div4_lo_inject     50
        0,   // div4_hi_read       50
        0,   // div4_hi_internal   50
        10,  // div4_hi_capture    50
};
static_assert(sizeof(DIV2_DEFAULTS) / sizeof(DIV2_DEFAULTS[0]) == DIV2_DELAYS);
static_assert(sizeof(TMS7002_DEFAULTS) / sizeof(TMS7002_DEFAULTS[0]) ==
              DIV2_DELAYS);
static_assert(sizeof(CMOS7002_DEFAULTS) / sizeof(CMOS7002_DEFAULTS[0]) ==
              DIV2_DELAYS);
static_assert(sizeof(DIV4_DEFAULTS) / sizeof(DIV4_DEFAULTS[0]) == DIV4_DELAYS);

template <EngineType E>
constexpr uint16_t defaultNs(Div2Delay f) {
    return E == ENG_CMOS_7002   ? CMOS7002_DEFAULTS[f]
           : E == ENG_DIV2_7002 ? TMS7002_DEFAULTS[f]
                                : DIV2_DEFAULTS[f];
}

constexpr uint16_t defaultNs(Div4Delay f) {
    return DIV4_DEFAULTS[f];
}


template <auto F>
inline void delay() {
    static_assert(!std::is_same_v<decltype(F), Div2Delay>,
            "a /2 delay takes its engine: delay<E, F>()");
    if constexpr (defaultNs(F) != 0)
        delayNanoseconds(defaultNs(F));
}

template <EngineType E, Div2Delay F>
inline void delay() {
    if constexpr (defaultNs<E>(F) != 0)
        delayNanoseconds(defaultNs<E>(F));
}

inline void clkin_hi() {
    digitalWriteFast(PIN_CLKIN, HIGH);
}

inline void clkin_lo() {
    digitalWriteFast(PIN_CLKIN, LOW);
}

inline auto signal_clkout() {
    return digitalReadFast(PIN_CLKOUT);
}

inline auto signal_alatch() {
    return digitalReadFast(PIN_ALATCH);
}

inline void assert_reset() {
    digitalWriteFast(PIN_RESET, LOW);
}

inline void negate_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

const uint8_t PINS_HIGH[] = {
        PIN_RESET,
        PIN_CLKIN,
        PIN_INT1,
        PIN_INT3,
};

const uint8_t PINS_INPUT[] = {
        PIN_AD0,
        PIN_AD1,
        PIN_AD2,
        PIN_AD3,
        PIN_AD4,
        PIN_AD5,
        PIN_AD6,
        PIN_AD7,
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
        PIN_CLKOUT,
        PIN_ENABLE,
        PIN_RW,
        PIN_ALATCH,
        PIN_PA0,
        PIN_PA1,
        PIN_PA2,
        PIN_PA3,
        PIN_PA4,
        PIN_PA5,
        PIN_PA6,
        PIN_PA7,
        PIN_PB0,
        PIN_PB1,
        PIN_PB2,
        PIN_PB3,
};

inline void assert_debug() {
    Pins::assert_debug();
}

inline void negate_debug() {
    Pins::negate_debug();
}

inline void toggle_debug() {
    Pins::toggle_debug();
}

inline void clkin_cycle() {
    clkin_hi();
    delayNanoseconds(clkin_hi_ns);
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
}

}  // namespace

template <EngineType E>
struct Traits {
    static constexpr bool div4 = E == ENG_DIV4;
    static constexpr bool cmos = E == ENG_CMOS || E == ENG_CMOS_7002;
    static constexpr bool tms7002 = E == ENG_DIV2_7002 || E == ENG_CMOS_7002;
    // A CMOS part is clocked on CLKIN inverted from the NMOS /2 parts (see
    // the diagram): clk_hi() drives CLKIN low.
    static constexpr uint8_t clkinHi = cmos ? LOW : HIGH;
    static constexpr uint8_t clkinLo = cmos ? HIGH : LOW;
};

// Calls |f| with the engine as a compile-time constant.
template <typename F>
auto PinsTms7000::withEngine(F f) const {
    switch (_engine) {
    case ENG_DIV4:
        return f(std::integral_constant<EngineType, ENG_DIV4>());
    case ENG_CMOS:
        return f(std::integral_constant<EngineType, ENG_CMOS>());
    case ENG_DIV2_7002:
        return f(std::integral_constant<EngineType, ENG_DIV2_7002>());
    case ENG_CMOS_7002:
        return f(std::integral_constant<EngineType, ENG_CMOS_7002>());
    default:
        return f(std::integral_constant<EngineType, ENG_DIV2>());
    }
}


// The /2 clock edges.
template <EngineType E>
inline void clk_hi(DevsTms7000 *devs) {
    digitalWriteFast(PIN_CLKIN, Traits<E>::clkinHi);
    devs->serialLoop();
}

template <EngineType E>
inline void clk_lo() {
    digitalWriteFast(PIN_CLKIN, Traits<E>::clkinLo);
}

template <EngineType E>
void div2_clk(DevsTms7000 *devs) {
    clk_hi<E>(devs);
    delay<E, div2_hi_ns>();
    clk_lo<E>();
    delay<E, div2_lo_ns>();
}

// Clocks to wait for ALATCH, pulsed every machine cycle but for a CMOS
// CPU in IDLE, which drives no bus cycle until an interrupt.
constexpr auto alatch_clocks = 1000;

// Gives up the wait rather than wedge the board, leaving the halt switch
// to stop the run.
void no_alatch() {
    cli.println("?halt: no bus cycle");
}

template <EngineType E>
void div2_alatch(DevsTms7000 *devs) {
    for (auto n = 0;; ++n) {
        clk_hi<E>(devs);
        delay<E, div2_hi_alatch>();
        if (signal_alatch() != LOW)
            break;
        if (n >= alatch_clocks)
            return no_alatch();
        clk_lo<E>();
        delay<E, div2_lo_alatch>();
    }
}

template <EngineType E>
Signals *div2Prepare(DevsTms7000 *devs) {
    auto s = Signals::put();
    // ALATCH=H
    clk_lo<E>();
    s->getAddress();
    delay<E, div2_lo_addr>();
    clk_hi<E>(devs);
    delay<E, div2_hi_ns>();
    clk_lo<E>();
    delay<E, div2_lo_dir>();
    s->getDirection();
    return s;
}

template <EngineType E>
void tms7002_alatch(DevsTms7000 *devs) {
    for (auto n = 0; signal_alatch() == LOW; ++n) {
        if (n >= alatch_clocks)
            return no_alatch();
        clk_hi<E>(devs);
        delay<E, tms7002_hi_alatch>();
        clk_lo<E>();
        delay<E, tms7002_lo_alatch>();
    }
}

template <EngineType E>
Signals *tms7002Prepare(DevsTms7000 *devs) {
    auto s = Signals::put();
    // ALATCH=H
    clk_hi<E>(devs);
    s->getAddress();
    delay<E, tms7002_hi_addr>();
    clk_lo<E>();
    delay<E, div2_lo_ns>();
    clk_hi<E>(devs);
    delay<E, div2_hi_ns>();
    clk_lo<E>();
    delay<E, div2_lo_dir>();
    s->getDirection();
    return s;
}

template <EngineType E>
void PinsTms7000::div2Complete(Signals *s) const {
    auto d = devs<DevsTms7000>();
    if (s->read()) {  // External read
        if (s->readMemory()) {
            s->data = mems<MemsTms7000>()->read(s->addr);
        } else {
            ;  // inject
        }
        s->outData();
        clk_hi<E>(d);
        delay<E, div2_hi_ns>();
        clk_lo<E>();
        delay<E, div2_lo_read>();
        s->inputMode();
    } else if (s->write()) {  // External write
        clk_hi<E>(d);
        delay<E, div2_hi_write>();
        s->getData();
        clk_lo<E>();
        if (s->writeMemory()) {
            mems<MemsTms7000>()->write(s->addr, s->data);
        } else {
            ;  // capture
        }
        delay<E, div2_lo_write>();
    } else {  // Internal cycle
        clk_hi<E>(d);
        s->getData();
        delay<E, div2_hi_internal>();
        clk_lo<E>();
    }
}

// Runs the serial handler on every other high phase; div4_hi_wait pads the
// rest to a similar length.
void div4_hi(DevsTms7000 *devs) {
    static bool serial = false;
    clkin_hi();
    serial = !serial;
    if (serial) {
        devs->serialLoop();
        delay<div4_hi_serial>();
    } else {
        delay<div4_hi_wait>();
    }
}

void div4_lo() {
    clkin_lo();
}

void div4_cycle_lo(DevsTms7000 *devs) {
    div4_hi(devs);
    div4_lo();
}

void div4_clk(DevsTms7000 *devs) {
    div4_cycle_lo(devs);
    delay<div4_lo_ns>();
    div4_cycle_lo(devs);
    delay<div4_lo_ns>();
}

void div4_alatch(DevsTms7000 *devs) {
    for (auto n = 0; signal_alatch() == LOW; ++n) {
        if (n >= alatch_clocks)
            return no_alatch();
        delay<div4_lo_alatch>();
        div4_cycle_lo(devs);
    }
}

Signals *div4Prepare(DevsTms7000 *devs) {
    auto s = Signals::put();
    do {
        delay<div4_lo_latched>();
        s->getAddress();
        div4_cycle_lo(devs);
    } while (signal_alatch() != LOW);
    delay<div4_lo_addr>();
    div4_cycle_lo(devs);
    delay<div4_lo_ns>();
    div4_hi(devs);
    s->getDirection();
    return s;
}

void PinsTms7000::div4Complete(Signals *s) const {
    if (s->read()) {  // External read
        if (s->readMemory()) {
            s->data = mems<MemsTms7000>()->read(s->addr);
        } else {
            delay<div4_lo_inject>();
        }
        s->outData();
        delay<div4_hi_read>();
        div4_lo();
        s->inputMode();
    } else if (s->write()) {  // External write
        div4_lo();
        delay<div4_lo_ns>();
        div4_hi(devs<DevsTms7000>());
        s->getData();
        div4_lo();
        if (s->writeMemory()) {
            mems<MemsTms7000>()->write(s->addr, s->data);
        } else {
            delay<div4_hi_capture>();
        }
    } else {  // Internal cycle
        s->getData();
        delay<div4_hi_internal>();
        div4_lo();
    }
}

template <EngineType E>
void clk_cycle(DevsTms7000 *devs) {
    if constexpr (Traits<E>::div4) {
        div4_clk(devs);
    } else {
        div2_clk<E>(devs);
    }
}

template <EngineType E>
void wait_alatch(DevsTms7000 *devs) {
    if constexpr (Traits<E>::div4) {
        div4_alatch(devs);
    } else if constexpr (Traits<E>::tms7002) {
        tms7002_alatch<E>(devs);
    } else {
        div2_alatch<E>(devs);
    }
}

template <EngineType E>
Signals *PinsTms7000::prepareCycle() const {
    const auto d = devs<DevsTms7000>();
    if constexpr (Traits<E>::div4) {
        return div4Prepare(d);
    } else if constexpr (Traits<E>::tms7002) {
        return tms7002Prepare<E>(d);
    } else {
        return div2Prepare<E>(d);
    }
}

PinsTms7000::PinsTms7000() {
    auto regs = new RegsTms7000(this);
    _regs = regs;
    auto devs = new DevsTms7000();
    _devs = devs;
    _mems = new MemsTms7000(regs, devs);
}

void PinsTms7000::resetPins() {
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    synchronizeClock();

    withEngine([&](auto e) {
        constexpr auto E = decltype(e)::value;
        // The reset sequence clears IOCNT0 (>0100), disabling interrupts
        cycle<E>();
        // Inject dummy reset vector >8000
        inject<E>(0x00);  // >FFFF
        inject<E>(0x80);  // >FFFE
    });
    // CLKOUT=H
    _regs->save();
    _regs->setIp(_mems->read16(InstTms7000::VEC_RESET));
    withEngine([&](auto e) { checkHardwareType<decltype(e)::value>(); });
}

// CLKIN cycles to wait for a CLKOUT edge, which comes every 2 or 4.
constexpr auto clkout_clocks = 64;

static bool wait_clkout(bool high) {
    for (auto n = 0; n < clkout_clocks; ++n) {
        if ((signal_clkout() != LOW) == high)
            return true;
        clkin_cycle();
    }
    cli.println("?halt: no CLKOUT");
    return false;
}

void PinsTms7000::synchronizeClock() {
    // A CMOS part in IDLE stops CLKOUT until a reset wakes it.
    assert_reset();
    for (auto i = 0; i < 80; i++)
        clkin_cycle();
    negate_reset();
    // CLKOUT works only when #RESET=H
    wait_clkout(false);
    wait_clkout(true);
    // CLKOUT=H
    clkin_cycle();  // /2:CLKOUT=L, /4:CLKOUT=H
    clkin_cycle();  // /2:CLKOUT=H, /4:CLKOUT=L
    if (signal_clkout() != LOW) {
        // /2 clock option
        clkin_hi();
        delayNanoseconds(clkin_hi_ns);
        if (signal_clkout() == LOW) {
            // NMOS device
            _clockType = CLK_DIV2;
            _engine = ENG_DIV2;
        } else {
            // CMOS device
            _clockType = CLK_CMOS;
            _engine = ENG_CMOS;
        }
    } else {
        // /4 clock option
        clkin_hi();
        delayNanoseconds(clkin_hi_ns);
        _clockType = CLK_DIV4;
        _engine = ENG_DIV4;
    }

    // Only the clock's engines: resetCpu() finds a TMS7002.
    switch (_engine) {
    case ENG_DIV4:
        resetCpu<ENG_DIV4>();
        break;
    case ENG_CMOS:
        resetCpu<ENG_CMOS>();
        break;
    default:
        resetCpu<ENG_DIV2>();
        break;
    }
}

template <EngineType E>
void PinsTms7000::resetCpu() {
    // The RESET function is initiated when the #RESET line of the
    // TMS7000 device is held at a logic zero level for at least five
    // clock cycles.
    auto d = devs<DevsTms7000>();
    assert_reset();
    for (auto i = 0; i < 20; i++)
        clk_cycle<E>(d);
    Cycles::reset();
    negate_reset();
    clk_cycle<E>(d);

    if constexpr (Traits<E>::div4) {
        wait_alatch<E>(d);
    } else {
        for (auto n = 0;; ++n) {
            if (n >= alatch_clocks) {
                no_alatch();
                break;
            }
            clk_hi<E>(d);
            delay<E, div2_hi_ns>();
            if (signal_alatch() != LOW)
                break;
            clk_lo<E>();
            delay<E, div2_lo_ns>();
            // ALATCH rising in the clk_lo() half is the TMS7002 timing.
            if (signal_alatch() != LOW) {
                _engine = Traits<E>::cmos ? ENG_CMOS_7002 : ENG_DIV2_7002;
                break;
            }
        }
    }
}

template <EngineType E>
void PinsTms7000::checkHardwareType() {
    inject<E>(0x8A);        // LDA @>0080
    inject<E>(hi(0x0080));  // TMS7000/TMS7001 have 128 bytes internal RAM
    inject<E>(lo(0x0080));  // TMS7002 has 256 bytes internal RAM
    const auto reg80 = cycle<E>();
    if (reg80->external()) {
        inject<E>(0x8A);        // LDA @>0110
        inject<E>(hi(0x0110));  // TMS7000 has no Serial peripheral
        inject<E>(lo(0x0110));  // TMS7001 has a Serial peripheral
        const auto port10 = cycle<E>();
        _hardType = port10->external() ? HW_TMS7000 : HW_TMS7001;
    } else {
        _hardType = HW_TMS7002;
    }
    regs<RegsTms7000>()->restoreA();
    if (_hardType != HW_TMS7000)
        devs<DevsTms7000>()->addSerialHandler(_hardType);
}

template <EngineType E>
Signals *PinsTms7000::completeCycle(Signals *s) const {
    if constexpr (Traits<E>::div4) {
        div4Complete(s);
    } else {
        div2Complete<E>(s);
    }
    Cycles::next();
    if constexpr (Traits<E>::div4) {
        delay<div4_cycle_delay>();
    } else {
        delay<E, cycle_delay>();
    }
    wait_alatch<E>(devs<DevsTms7000>());
    return s;
}

template <EngineType E>
Signals *PinsTms7000::cycle() const {
    return completeCycle<E>(prepareCycle<E>());
}

template <EngineType E>
Signals *PinsTms7000::inject(uint8_t data) const {
    return completeCycle<E>(prepareCycle<E>()->inject(data));
}

void PinsTms7000::execInst(const uint8_t *inst, uint8_t len, uint8_t extra) {
    withEngine([&](auto e) {
        constexpr auto E = decltype(e)::value;
        execute<E>(inst, len);
        for (uint8_t i = 0; i < extra; ++i)
            cycle<E>();
    });
}

uint16_t PinsTms7000::captureWrites(const uint8_t *inst, uint8_t len, uint8_t *buf,
        uint8_t max) {
    return withEngine([&](auto e) {
        return execute<decltype(e)::value>(inst, len, buf, max);
    });
}

template <EngineType E>
uint16_t PinsTms7000::execute(const uint8_t *inst, uint8_t len, uint8_t *buf,
        uint8_t max) {
    uint16_t addr = 0;
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = _parked ? _parked : prepareCycle<E>();
        _parked = nullptr;
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle<E>(s);
        if (s->read()) {
            if (inj == 0)
                addr = s->addr;
            if (inj < len)
                ++inj;
        } else if (s->write()) {
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
    }
    return addr;
}

// JMP $ runs without an interrupt this many times before a save: one
// pending at the halt is taken at the first boundary, and one recognized a
// boundary late at the second.
constexpr auto park_jumps = 2;

// An interrupt entry shows only an acknowledge cycle and its two vector
// reads, with the stack on chip, and the TMS70C02 can be at the vector reads
// already at the halt. JMP $ runs at the halt until it has run park_jumps
// times without one, or an interrupt is taken, which masks the rest: the
// save then starts at the handler's fetch, with the program's PC and ST
// stacked.
void PinsTms7000::park() {
    withEngine([&](auto e) { park<decltype(e)::value>(); });
}

template <EngineType E>
void PinsTms7000::park() {
    auto s = prepareCycle<E>();
    for (auto jumps = 0;; ++jumps) {
        if (s->intack()) {
            completeCycle<E>(s);
            s = prepareCycle<E>();
        }
        if (s->read() && s->addr >= InstTms7000::VEC_TRAP23) {
            completeCycle<E>(s);  // lo(vector)
            cycle<E>();           // hi(vector)
            s = prepareCycle<E>();
            break;
        }
        if (jumps == park_jumps)
            break;
        completeCycle<E>(s->inject(InstTms7000::JMP));
        inject<E>(InstTms7000::JMP_HERE);
        s = prepareCycle<E>();
    }
    _parked = s;
}

void PinsTms7000::idle() {
    withEngine([&](auto e) {
        constexpr auto E = decltype(e)::value;
        auto s = inject<E>(InstTms7000::JMP);
        inject<E>(InstTms7000::JMP_HERE);
        Cycles::discard(s);
    });
}

#ifdef PROFILE_CYCLES
// For scripts/record-cycles.py: run until a TRAP 23 reads its vector, keep
// every cycle of it, and give up well before the ring wraps; the debug pin
// frames the run, to trigger a capture on. The stop doesn't consult the
// tables, so it holds wherever the matcher goes wrong.
template <EngineType E>
bool PinsTms7000::loop() {
    constexpr auto MAX_CYCLES = 96;
    const auto devs = this->devs<DevsTms7000>();
    _profileEnd = nullptr;
    assert_debug();
    for (auto n = 0;; ++n) {
        devs->loop();
        auto s = prepareCycle<E>();
        if (isTrap23Vector(s)) {
            // Its pushes stay in the register file, off the bus: the TRAP's
            // fetch is the cycle before.
            if (n > 0)
                _profileEnd = s->prev();
            completeCycle<E>(s);
            s = prepareCycle<E>();
            if (isTrap23Vector(s)) {
                completeCycle<E>(s);
                s = prepareCycle<E>();
            }
            negate_debug();
            // Halt at the handler's fetch, as rawStep() does at IDLE.
            completeCycle<E>(s->inject(InstTms7000::JMP));
            inject<E>(InstTms7000::JMP_HERE);
            Cycles::discard(s);
            return true;
        }
        completeCycle<E>(s);
        if (n >= MAX_CYCLES || haltSwitch()) {
            negate_debug();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            return false;
        }
    }
}

bool PinsTms7000::isTrap23Vector(const Signals *s) {
    return s->read() && (s->addr | 1) == (InstTms7000::VEC_TRAP23 | 1);
}
#else
template <EngineType E>
void PinsTms7000::loop() {
    const auto devs = this->devs<DevsTms7000>();
    while (true) {
        const auto serviced = devs->loop();
        if (!rawStep<E>() || (serviced && haltSwitch()))
            return;
    }
}
#endif

void PinsTms7000::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
#ifdef PROFILE_CYCLES
    const auto stopped =
            withEngine([&](auto e) { return loop<decltype(e)::value>(); });
#else
    withEngine([&](auto e) { loop<decltype(e)::value>(); });
#endif
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
#ifdef PROFILE_CYCLES
    // A run that gave up leaves the CPU mid-instruction, where saving would
    // wait for writes that may never come: keep the last good save.
    if (!stopped)
        return;
#endif
    _regs->save();
}

template <EngineType E>
bool PinsTms7000::rawStep() {
    auto s = prepareCycle<E>();
    if (s->intack()) {  // Interrupt
    interrupt:
        completeCycle<E>(s);
        cycle<E>();  // lo(vector)
        cycle<E>();  // hi(vector)
        s = prepareCycle<E>();
    }
    const auto opc = mems<MemsTms7000>()->read_byte(s->addr);
    const auto cycles = InstTms7000::busCycles(opc);
    // Stop at a break (IDLE) or an opcode the table lacks: JMP $ in its
    // place leaves PC at its fetch.
    if (opc == InstTms7000::IDLE || cycles == 0) {
        completeCycle<E>(s->inject(InstTms7000::JMP));
        inject<E>(InstTms7000::JMP_HERE);
        Cycles::discard(s);
        return false;
    }
    auto fetch = completeCycle<E>(s);
    for (uint8_t i = 1; i < cycles; ++i) {
        s = prepareCycle<E>();
        if (s->intack())
            goto interrupt;
        completeCycle<E>(s);
    }
    fetch->markFetch(true);
    return true;
}

bool PinsTms7000::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    if (withEngine([&](auto e) { return rawStep<decltype(e)::value>(); })) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsTms7000::assertInt(uint8_t name) {
    switch (name) {
    case INTR_INT1:
        digitalWriteFast(PIN_INT1, LOW);
        break;
    case INTR_INT3:
        digitalWriteFast(PIN_INT3, LOW);
        break;
    case INTR_NONE:
        break;
    }
}

void PinsTms7000::negateInt(uint8_t name) {
    switch (name) {
    case INTR_INT1:
        digitalWriteFast(PIN_INT1, HIGH);
        break;
    case INTR_INT3:
        digitalWriteFast(PIN_INT3, HIGH);
        break;
    case INTR_NONE:
        break;
    }
}

void PinsTms7000::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstTms7000::IDLE);
}

void PinsTms7000::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsTms7000::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

#ifdef PROFILE_CYCLES
// The fetches rawStep() would mark, counting each instruction's bus cycles
// from the run's first cycle.
void PinsTms7000::markFetches(const Signals *end) {
    const auto g = Signals::get();
    const auto cycles = g->diff(end);
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        const auto n = InstTms7000::busCycles(s->data);
        s->markFetch(true);
        s->setMatched(n);
        if (n == 0)
            break;
        i += n;
    }
}
#endif

void PinsTms7000::printBacktrace() {
#ifdef PROFILE_CYCLES
    // Every cycle, with the matcher's marks up to the TRAP's fetch that ends
    // the pattern.
    markFetches(_profileEnd ? _profileEnd->next() : Signals::put());
    printCycles();
#else
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            i += len;
            if (InstTms7000::isBTJxP(s->data)) {
                s->next(len - 1)->print();
                i += 1;
            }
        } else {
            s->print();
            ++i;
        }
        idle();
    }
#endif
}

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

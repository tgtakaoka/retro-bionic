#include "pins_mos6502.h"
#include <type_traits>
#include "debugger.h"
#include "devs_mos6502.h"
#include "inst_mos6502.h"
#include "mems_mos6502.h"
#include "regs_mos6502.h"
#include "signals_mos6502.h"

namespace debugger {
namespace mos6502 {

/**
 * W65C02S bus cycle.
 *         __________            __________            ______
 * PHI2  _|          |__________|          |__________|
 *       __            __________            __________
 * PHI1O   |__________|          |__________|          |_____
 *           __________            __________            ____
 * PHI2O ___|          |__________|          |__________|
 *       ________  ____________________  ____________________
 *  SYNC ________><____________________><____________________
 *                ______________________
 *   R/W --------<                      |____________________
 *                           ____               _________
 *  Data -------------------<rrrr>-------------<wwwwwwwww>---
 */

namespace {
// Each part's delays start from its own datasheet column at 5 V: the
// NMOS 6502 from the UM6502's 2 MHz one. On the NMOS pinout PHI2 follows
// PHI0 by up to tDLY; the W65Cxx take PHI0 as PHI2 itself. Each delay is
// a minimum that the code between the edges pads; 20 ns covers the base
// board's level shifters.
//
//                 UM6502A G65SC02 R65C02 W65C02S W65C816S
//                   2 MHz   4 MHz  3 MHz  14 MHz  14 MHz
// tDLY  max ns         65      40     40       -       -  PHI0 to PHI2
// tPWL  min ns        240     100    150      35      35  PHI2 low
// tPWH  min ns        235     110    160      35      35  PHI2 high
// tADS  max ns        140      70     85      30      33  addr, BA
// tSYS  max ns        175      70     85      30      30  SYNC, VPA/VDA
// tDSR  min ns         50      30     40      10      10  read data setup
// tMDS  max ns        100      55     85      25      30  write data
// tBH   min ns          -       -      -       -      10  BA hold

inline void phi0_hi() {
    digitalWriteFast(PIN_PHI0, HIGH);
}

inline void phi0_lo() {
    digitalWriteFast(PIN_PHI0, LOW);
}

auto signal_e() {
    return digitalReadFast(PIN_E);
}

auto signal_phi1o() {
    return digitalReadFast(PIN_PHI1O);
}

auto signal_vp() {
    return digitalReadFast(PIN_VP);
}

void assert_irq() {
    digitalWriteFast(PIN_IRQ, LOW);
}

void negate_irq() {
    digitalWriteFast(PIN_IRQ, HIGH);
}

void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline bool asserted(uint8_t pin) {
    return (*portOutputRegister(pin) & digitalPinToBitMask(pin)) == 0;
}

void assert_rdy() {
    pinMode(PIN_RDY, INPUT_PULLUP);
    // The pull-up takes about 350 ns to bring RDY to a CMOS high.
    delayNanoseconds(500);
}

void negate_rdy() {
    pinMode(PIN_RDY, OUTPUT_OPENDRAIN);
    digitalWriteFast(PIN_RDY, LOW);
}

void assert_reset() {
    digitalWriteFast(PIN_RES, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RES, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_PHI0,
        PIN_RES,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_NMI,
        PIN_IRQ,
};

constexpr uint8_t PINS_PULLUP[] = {
        // RDY(input) for 6502, pullup
        // RDY(bi-directional) for W65Cxx, pullup
        PIN_RDY,
        // PHI1O(output) from 6502
        // #ABORT(input) for 65816, pullup
        PIN_PHI1O,
        // NC for 6502
        // #ML(output) from W65Cxx
        PIN_ML,
        // #SO(input) for 6502, pullup
        // #MX(output) from 65816
        PIN_SO,
        // NC on 6502
        // E(output) from 65816
        PIN_E,
        // VSS for 6502
        // #VP(output) from W65Cxx
        PIN_VP,
        // NC on 6502
        // BE(input) for W65Cxx
        PIN_BE,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_AL0,
        PIN_AL1,
        PIN_AL2,
        PIN_AL3,
        PIN_AL4,
        PIN_AL5,
        PIN_AL6,
        PIN_AL7,
        PIN_AM8,
        PIN_AM9,
        PIN_AM10,
        PIN_AM11,
        PIN_AH12,
        PIN_AH13,
        PIN_AH14,
        PIN_AH15,
        PIN_RW,
        // SYNC(output) from 6502
        // VPA(output) from 65816
        PIN_SYNC,
        // PHI2O(output) from 6502
        // VDA(output) from 65816
        PIN_PHI2O,
};

}  // namespace

#ifdef PROFILE_CYCLES
// Where a run's bus cycles spend their time: the cycle counter at each
// point since the previous PHI0 fall, summed apart for fetches.
namespace profile {
enum Point : uint8_t {
    NEXT,     // Cycles::next() done
    DEVS,     // devs->loop() done
    LOFETCH,  // loFetch done
    CONTROL,  // control read
    ADDRESS,  // address read
    DECIDE,   // fetch read and break check, or not a fetch
    RISE,     // PHI0 rise stored
    READ,     // memory read and data staged, or data written
    DRIVE,    // data driven
    RELEASE,  // data released
    FALL,     // PHI0 fall stored: the cycle's length
    POINTS,
};
uint32_t fall;
uint32_t at[POINTS];
bool fetch;
uint64_t sum[2][POINTS];
uint32_t count[2];

inline void mark(Point p) {
    at[p] = ARM_DWT_CYCCNT - fall;
}

inline void phi0Fall() {
    const auto now = ARM_DWT_CYCCNT;
    at[FALL] = now - fall;
    fall = now;
    for (auto p = 0; p < POINTS; p++)
        sum[fetch][p] += at[p];
    count[fetch]++;
    fetch = false;
}

void reset() {
    fall = ARM_DWT_CYCCNT;
    fetch = false;
    for (auto f = 0; f < 2; f++) {
        count[f] = 0;
        for (auto p = 0; p < POINTS; p++)
            sum[f][p] = 0;
    }
}

void print() {
    static const char *const NAMES[POINTS] = {"next", "devs", "lofetch",
            "control", "address", "decide", "rise", "read", "drive", "release",
            "fall"};
    for (auto f = 0; f < 2; f++) {
        cli.print(f ? "fetch " : "other ");
        cli.printDec(count[f]);
        cli.println(" cycles, ns after PHI0 fall:");
        if (count[f] == 0)
            continue;
        for (auto p = 0; p < POINTS; p++) {
            cli.print("  ");
            cli.printStr(NAMES[p], -8);
            cli.printDec(uint32_t(sum[f][p] * 1000 / count[f] /
                                  (F_CPU_ACTUAL / 1000000)));
            cli.println();
        }
    }
}
}  // namespace profile
#define PROFILE_MARK(p) profile::mark(profile::p)
#define PROFILE_FALL() profile::phi0Fall()
#else
#define PROFILE_MARK(p) \
    do {                \
    } while (0)
#define PROFILE_FALL() \
    do {               \
    } while (0)
#endif

// Delays by SoftwareType, in ns, from the datasheet columns above, with
// the run loop's PHI0 low and the PHI0 high ones cut to what passed the
// full regress on the bench (2026-11-07): the code between the edges
// makes up the rest of each minimum.
constexpr PinsMos6502::Timing TIMINGS[] = {
        // SW_MOS6502: tSYS+tDLY; tDSR+tDLY; tMDS+tDLY, filled to tPWH;
        // loFetch halved.
        {260, 0, 0, 130, 0, 0, 60, 120, 100, 190, 40},
        // SW_G65SC02: no loFetch, the PHI0 high ones halved.
        {130, 0, 0, 0, 0, 0, 0, 45, 15, 60, 0},
        // SW_R65C02: loFetch a tenth, the PHI0 high ones halved but
        // hiReadPost and hiWrite, which PHI0 high needs for tPWH.
        {150, 0, 0, 15, 0, 0, 0, 90, 30, 90, 10},
        // SW_W65C02S: no loFetch, the PHI0 high ones a tenth.
        {50, 0, 0, 0, 0, 0, 0, 3, 0, 5, 0},
        // SW_W65C816: data out tBH after PHI2 rises; all but lo halved.
        {50, 0, 0, 25, 0, 0, 10, 15, 0, 25, 0},
};

// The datasheet columns the delays come from. K scales the table as if
// the part were rated at another clock: K at these leaves it as is.
constexpr uint32_t RATED_HZ[] = {
        2000000,   // SW_MOS6502
        4000000,   // SW_G65SC02
        3000000,   // SW_R65C02
        14000000,  // SW_W65C02S
        14000000,  // SW_W65C816
};

template <SoftwareType E>
struct PinsMos6502::Traits {
    static constexpr HardwareType hard = E == SW_W65C816   ? HW_W65C816
                                         : E == SW_W65C02S ? HW_W65C02S
                                                           : HW_MOS6502;
    // BA rides D0-D7 while PHI2 is low.
    static constexpr bool bankOnData = hard == HW_W65C816;
    static constexpr Timing timing = TIMINGS[E];
};

template <SoftwareType E, uint16_t PinsMos6502::Timing::*F>
inline void PinsMos6502::delay() const {
    if constexpr (Traits<E>::timing.*F != 0) {
#ifdef TUNE_CLOCK
        delayNanoseconds(_timing.*F);
#else
        delayNanoseconds(Traits<E>::timing.*F);
#endif
    }
}

#ifdef TUNE_CLOCK
uint32_t PinsMos6502::clockInput() const {
    return uint64_t(RATED_HZ[_engine]) * _clockPermille / 1000;
}

// Scales the engine's delays as if its part ran at |hz|.
bool PinsMos6502::setClockInput(uint32_t hz) {
    const auto permille = uint64_t(hz) * 1000 / RATED_HZ[_engine];
    if (permille < 100 || permille > 10000)
        return false;
    _clockPermille = permille;
    setTiming();
    return true;
}

void PinsMos6502::setTiming() {
    const auto base = reinterpret_cast<const uint16_t *>(&TIMINGS[_engine]);
    auto ns = reinterpret_cast<uint16_t *>(&_timing);
    for (auto i = 0u; i < sizeof(Timing) / sizeof(uint16_t); i++)
        ns[i] = uint32_t(base[i]) * 1000 / _clockPermille;
}
#endif

// Calls |f| with the engine as a compile-time constant.
template <typename F>
auto PinsMos6502::withEngine(F f) {
    switch (_engine) {
    case SW_W65C816:
        return f(std::integral_constant<SoftwareType, SW_W65C816>());
    case SW_W65C02S:
        return f(std::integral_constant<SoftwareType, SW_W65C02S>());
    case SW_R65C02:
        return f(std::integral_constant<SoftwareType, SW_R65C02>());
    case SW_G65SC02:
        return f(std::integral_constant<SoftwareType, SW_G65SC02>());
    default:
        return f(std::integral_constant<SoftwareType, SW_MOS6502>());
    }
}

PinsMos6502::PinsMos6502() {
    auto devs = new DevsMos6502();
    _devs = devs;
    auto mems = new MemsMos6502(devs);
    _mems = mems;
    _regs = new RegsMos6502(this, mems);
}

bool PinsMos6502::native65816() const {
    return _hardType == HW_W65C816 && signal_e() == LOW;
}

HardwareType PinsMos6502::_hardType;

void PinsMos6502::checkHardwareType() {
    // A #RES pulse negates #VP on a W65Cxx; the pin is Vss on the others.
    negate_reset();
    phi0_hi();
    delayNanoseconds(500);
    assert_reset();
    if (signal_phi1o() == LOW) {
        // PIN_PHI1O follows PHI0 inverted: not the W65C816's #ABORT.
        if (signal_vp() == LOW) {
            // PIN_VP keeps LOW, means Vss of MOS6502, G65SC02, R65C02.
            _hardType = HW_MOS6502;
        } else {
            // PIN_VP keeps HIGH, means #VP of W65C02S.
            _hardType = HW_W65C02S;
            // BE is enabled by pullup
        }
        // Keep PIN_SO HIGH using pullup
    } else {
        // PIN_PHI1O/W65C816_ABORT keeps HIGH, means W65C816S.
        _hardType = HW_W65C816;
        // Negate #ABORT
        digitalWriteFast(W65C816_ABORT, HIGH);
        pinMode(W65C816_ABORT, OUTPUT);
        // BE is enabled by pullup
    }
}

SoftwareType PinsMos6502::_softType;

template <SoftwareType E>
void PinsMos6502::checkSoftwareType() {
    if (_hardType == HW_W65C816) {
        _softType = SW_W65C816;
        return;
    }

    // Tell the parts apart by where opcodes some of them lack take the bus.
    inject<E>(InstMos6502::JMP);
    inject<E>(0x00);
    inject<E>(0x10);
    auto base = inject<E>(InstMos6502::BRA);  // 4 clock branch
    inject<E>(InstMos6502::NOP);
    inject<E>(InstMos6502::NOP);
    inject<E>(InstMos6502::NOP);
    auto det65 = inject<E>(InstMos6502::JMP);
    inject<E>(0x00);
    inject<E>(0x10);
    // $80 is a two-byte NOP on the NMOS 6502.
    if (det65->addr == base->addr + 3U) {
        _softType = SW_MOS6502;
        return;
    }
    base = inject<E>(InstMos6502::BBR0);  // 6 clock branch
    inject<E>(InstMos6502::NOP);
    inject<E>(InstMos6502::NOP);
    inject<E>(InstMos6502::NOP);
    inject<E>(InstMos6502::NOP);
    inject<E>(InstMos6502::NOP);
    inject<E>(InstMos6502::NOP);
    det65 = inject<E>(InstMos6502::NOP);
    inject<E>(InstMos6502::NOP);
    // The G65SC02 has no BBR0 and runs $0F as a one-byte NOP.
    if (det65->addr == base->addr + 4) {
        _softType = SW_G65SC02;
    } else {
        _softType = _hardType == HW_MOS6502 ? SW_R65C02 : SW_W65C02S;
    }
}

void PinsMos6502::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    Signals::saveDataMode();

    checkHardwareType();
    // The hardware type's engine serves every part it covers.
    setEngine(static_cast<SoftwareType>(_hardType));
    withEngine([&](auto e) { resetCpu<decltype(e)::value>(); });
    setEngine(_softType);
}

void PinsMos6502::setEngine(SoftwareType engine) {
    _engine = engine;
#ifdef TUNE_CLOCK
    setTiming();
#endif
}

template <SoftwareType E>
void PinsMos6502::resetCpu() {
    // #RES must be held low for at least two clock cycles.
    for (auto i = 0; i < 10; i++)
        cycle<E>();
    auto s = prepareCycle<E>();
    negate_reset();
    completeCycle<E>(s);
    Cycles::reset();
    const auto reset_vec = _mems->read16(InstMos6502::VECTOR_RESET);
    _mems->write16(InstMos6502::VECTOR_RESET, 0x1000);  // dummy vector
    // When a positive edge is detected, there is an initialization
    // sequence lasting seven clock cycles.
    for (auto i = 0; i < 10; i++) {
        // there may be spurious write
        const auto s = completeCycle<E>(prepareCycle<E>()->capture());
        // Read dummy reset vector
        if (s->template vector<Traits<E>::hard>() &&
                s->addr == InstMos6502::VECTOR_RESET + 1)
            break;
    }

    _regs->save();
    assert_rdy();
    checkSoftwareType<E>();
    negate_rdy();
    _regs->setIp(reset_vec);
    _mems->write16(InstMos6502::VECTOR_RESET, reset_vec);
}

template <SoftwareType E>
inline Signals *PinsMos6502::rawPrepareCycle() {
    auto s = Signals::put();
    s->getControl();
    PROFILE_MARK(CONTROL);
    if constexpr (Traits<E>::bankOnData) {
        // E, this cycle's: whether BA carries a bank.
        if (s->addr24()) {
            s->getAddr24();
            PROFILE_MARK(ADDRESS);
            return s;
        }
    }
    delay<E, &Timing::loSync>();
    s->getAddr16();
    PROFILE_MARK(ADDRESS);
    return s;
}

template <SoftwareType E>
Signals *PinsMos6502::prepareCycle() {
    delay<E, &Timing::lo>();
    return rawPrepareCycle<E>();
}

template <SoftwareType E>
inline Signals *PinsMos6502::completeCycle(Signals *s) {
    phi0_hi();
    PROFILE_MARK(RISE);
    if (s->write()) {
        delay<E, &Timing::hiWrite>();
        s->getData();
        if (s->writeMemory()) {
            mems<MemsMos6502>()->write(s->addr, s->data);
        } else {
            delay<E, &Timing::hiCapture>();
        }
        PROFILE_MARK(READ);
        phi0_lo();
        PROFILE_FALL();
    } else {
        // Staged ahead of the delays, which the port writes drain during.
        if (s->readMemory()) {
            s->data = mems<MemsMos6502>()->read(s->addr);
            s->stageData();
        } else {
            s->stageData();
            delay<E, &Timing::hiInject>();
        }
        PROFILE_MARK(READ);
        // [W65C816] Delay to avoid bus conflict with bank address of
        // this bus cycle.
        delay<E, &Timing::hiReadPre>();
        Signals::outputMode();
        PROFILE_MARK(DRIVE);
        delay<E, &Timing::hiReadPost>();
        // [W65C816] Switch bus direction before falling PHI0 to avoid
        // bus conflict with bank address of next bus cycle. The
        // output data are retained by the bus-hold circuit until bank
        // address is on the bus.
        Signals::inputMode();
        PROFILE_MARK(RELEASE);
        phi0_lo();
        PROFILE_FALL();
    }
    Cycles::next();
    PROFILE_MARK(NEXT);

    return s;
}

template <SoftwareType E>
Signals *PinsMos6502::cycle() {
    return completeCycle<E>(prepareCycle<E>());
}

template <SoftwareType E>
Signals *PinsMos6502::inject(uint8_t data) {
    return completeCycle<E>(prepareCycle<E>()->inject(data));
}

Signals *PinsMos6502::inject(uint8_t data) {
    return withEngine([&](auto e) { return inject<decltype(e)::value>(data); });
}

void PinsMos6502::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsMos6502::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    execute(inst, len, addr, buf, max);
}

void PinsMos6502::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    withEngine([&](auto e) {
        execute<decltype(e)::value>(inst, len, addr, buf, max);
    });
}

template <SoftwareType E>
void PinsMos6502::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    assert_rdy();
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = prepareCycle<E>();
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        delay<E, &Timing::loExecute>();
        completeCycle<E>(s);
        if (s->read()) {
            if (inj < len)
                ++inj;
        } else {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
    }
    negate_rdy();
}

void PinsMos6502::idle() {
    // CPU is stopping with RDY=L
    delayNanoseconds(370);
    phi0_hi();
    delayNanoseconds(474);
    phi0_lo();
}

template <SoftwareType E>
bool PinsMos6502::loop() {
    const auto devs = this->devs<DevsMos6502>();
    const auto mems = this->mems<MemsMos6502>();
    // Devs::loop() reads the cycle counter to pace its service: asking it
    // every POLL_CYCLES bus cycles spares the others that read.
    constexpr uint_fast8_t POLL_CYCLES = 16;
    uint_fast8_t poll = 1;
    while (true) {
        auto serviced = false;
        if (--poll == 0) {
            poll = POLL_CYCLES;
            serviced = devs->loop();
        }
        PROFILE_MARK(DEVS);
        if (!serviced)
            delay<E, &Timing::loPoll>();
        delay<E, &Timing::loFetch>();
        PROFILE_MARK(LOFETCH);
        auto s = rawPrepareCycle<E>();
        if (s->template fetch<Traits<E>::hard>()) {
#ifdef PROFILE_CYCLES
            profile::fetch = true;
#endif
            const auto inst = mems->read(s->addr);
            if (inst == InstMos6502::BRK) {
                const auto opr = mems->read_byte(s->addr + 1);
                if (opr == 0 || isBreakPoint(s->addr))
                    return suspend<E>();
            }
            // Feed the opcode just read rather than read it again.
            s->inject(inst);
        } else {
            delay<E, &Timing::loLoop>();
        }
        PROFILE_MARK(DECIDE);
        completeCycle<E>(s);
        if (serviced && haltSwitch())
            return suspend<E>();
    }
}

void PinsMos6502::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    assert_rdy();
    startRunTimer();
#ifdef PROFILE_CYCLES
    profile::reset();
#endif
    const auto stopped =
            withEngine([&](auto e) { return loop<decltype(e)::value>(); });
    stopRunTimer();
    negate_rdy();
    restoreBreakInsts();
    disassembleCycles();
#ifdef PROFILE_CYCLES
    profile::print();
#endif
    // A failed halt leaves the CPU stopped mid-instruction: keep the
    // registers from the last good save.
    if (stopped)
        _regs->save();
}

// Cycles to take an NMI: the instruction under way, an IRQ entry the NMI
// may follow, the handler's first instruction, then the NMI entry. WAI
// wakes on the NMI; STP and the NMOS JAM opcodes never fetch again.
constexpr auto nmi_cycles = 64;

// Stop through an NMI, which masks IRQ and stacks PC and P itself, and
// leave the CPU at the handler's first fetch, not completed. The next
// opcode fetch is fed a JMP to itself until the NMI is taken, so the
// frame's PC is that fetch's address.
template <SoftwareType E>
bool PinsMos6502::suspend() {
    _frame.len = 0;
    assert_nmi();
    const Signals *first = nullptr;  // the first fed fetch
    uint32_t park = 0;
    auto parked = false;
    const Signals *push = nullptr;  // the first of consecutive writes
    uint8_t writes = 0;
    for (auto n = 0; n < nmi_cycles; ++n) {
        auto s = prepareCycle<E>();
        if (s->template fetch<Traits<E>::hard>()) {
            if (first == nullptr) {
                first = s;
                park = s->addr;
                parked = true;
            } else if (s->addr != park) {
                parked = false;  // an IRQ entry took it elsewhere
            }
        }
        if (parked && s->read() && s->addr - park < 3) {
            const uint8_t JMP_HERE[] = {InstMos6502::JMP, lo(park), hi(park)};
            s->inject(JMP_HERE[s->addr - park]);
        }
        const auto native = s->addr == InstMos6502::W65816_NMI;
        const uint8_t len = native ? 4 : 3;
        if (s->read() && writes >= len &&
                (native || s->addr == InstMos6502::VECTOR_NMI)) {
            negate_nmi();
            const auto frame = push->next(writes - len);
            _frame.len = len;
            _frame.sp = frame->addr;
            for (auto i = 0; i < len; ++i)
                _frame.data[i] = frame->next(i)->data;
            // The NMI entry starts with a discarded fetch and a dummy read
            Cycles::discard(parked ? first : frame->prev(2));
            // Any vector will do: the save is injected at its fetch
            inject<E>(lo(park));
            inject<E>(hi(park));
            return true;
        }
        completeCycle<E>(s);
        if (s->write()) {
            if (writes++ == 0)
                push = s;
        } else {
            writes = 0;
        }
    }
    negate_nmi();
    cli.println("?halt: no NMI");
    return false;
}

template <SoftwareType E>
bool PinsMos6502::rawStep() {
    auto s = rawPrepareCycle<E>();
    const auto inst = mems<MemsMos6502>()->read_byte(s->addr);
    if (InstMos6502::isStop(inst))
        return false;
    if (inst == InstMos6502::BRK) {
        const auto opr = mems<MemsMos6502>()->read_byte(s->addr + 1);
        if (opr == 0 || isBreakPoint(s->addr))
            return false;
    }
    assert_rdy();
    completeCycle<E>(s);
    // A pending IRQ the restored P lets in starts its entry before any
    // instruction, and an NMI raised during it would take its vector: let
    // the IRQ fetch its vector first, so the step stops at its handler.
    if (asserted(PIN_IRQ) && !regs<RegsMos6502>()->irqMasked()) {
        for (auto n = 0; n < nmi_cycles; ++n) {
            const auto v = cycle<E>();
            if (v->read() && (v->addr == InstMos6502::VECTOR_IRQ + 1 ||
                                     v->addr == InstMos6502::W65816_IRQ + 1))
                break;
        }
    }
    if (!suspend<E>()) {
        negate_rdy();
        return false;
    }
    negate_rdy();
    return true;
}

bool PinsMos6502::step(bool show) {
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

void PinsMos6502::assertInt(uint8_t) {
    assert_irq();
}

void PinsMos6502::negateInt(uint8_t) {
    negate_irq();
}

void PinsMos6502::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstMos6502::BRK);
}

void PinsMos6502::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; i++) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsMos6502::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsMos6502::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            i += len;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

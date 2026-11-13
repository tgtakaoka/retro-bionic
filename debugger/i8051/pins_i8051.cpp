#include "pins_i8051.h"
#include <type_traits>
#include "debugger.h"
#include "devs_i8051.h"
#include "inst_i8051.h"
#include "mems_i8051.h"
#include "regs_i8051.h"
#include "signals_i8051.h"

namespace debugger {
namespace i8051 {

// clang-format off
/**
 * P8051 bus cycle.
 *                                |   S4  |   S5  |   S6  |   S1  |   S2  |   S3  |   S1  |
 *        |   S1  |   S2  |   S3  |   S1  |       |       |       |       |       |       |
 *        |  _   _|  _   _|  _   _|  _   _|  _   _|  _   _|  _   _|  _   _|  _   _|  _   _|
 *  XTAL _|_|1|_|2|_|1|_|2|_|1|_|2|_|1|_|2|_|1|_|2|_|1|_|2|_|1|_|2|_|1|_|2|_|1|_|2|_|1|_|2|
 *            \ ______\   \           \ ______\           \                       \   \ __
 *   ALE ______|      |____|___________|       |___________|_______________________|___|
 *       ______|___________|           |___________________|_______________________|______
 * #PSEN ______|           |___________|                   |                       |
 *       __________________________________________________|                       |_______
 *   #RD                                                   |_______________________|
 *                  ______          ____     ______        |                 ______|
 *    P0 ----------<_addr_>--------<inst>---<_addr_>-------|----------------<_data_>------
 *       __________________________________________________|                       |______
 *   #WR                                                   |_______________________|
 *                  ______          ____     ______     ___________________________
 *    P0 ----------<_addr_>--------<inst>---<_addr_>---<____________data___________>------
 */
// clang-format on

namespace {
// 1/TCLCL: min  83 ns; clock period (12 MHz)
//   TLHLL: min 127 ns; ALE width
//   TAVLL: min  43 ns; Address valid to ALE-
//   TLLAX: min  48 ns; Address hold from ALE-
//   TPLPH: min 215 ns; #PSEN width
//   TLLPL: min  58 ns; ALE- to #PSEN-
//   TPLIV: max 125 ns; #PSEN- to Instr in
//   TPXIX: min   0 ns; Instr hold after #PSEN+
//   TPXIZ: max  63 ns; Instr float after #PSEN+
//   TPLAZ: max  20 ns; #PSEN- to Address float
//   TPXAV: min  75 ns; #PSEN+ to Address
//   TAVIV: max 302 ns; Address to Instr in
//   TRLRH: min 400 ns; #RD width
//   TWLWH: min 400 ns; #WR width
//   TRLDV: max 252 ns; #RD- to data in
//   TRHDX: min   0 ns; Data hold after #RD+
//   TRHDZ: max  97 ns; Data float after #RD+
//   TWHLH: max 123 ns; #RD/#WR+ to ALE+
//   TWHQX: min  33 ns; Data hold after #WR+
enum Delay : uint8_t {
    xtal_hi_ns,
    xtal_lo_ns,
    xtal_lo_addr,
    xtal_hi_ale,
    xtal_lo_cntl,
    xtal_hi_cntl,
    xtal_lo_inject,
    xtal_lo_output,
    xtal_hi_capture,
    xtal_hi_output,
    xtal_lo_data,
    xtal_hi_input,
    // Phases with no delay of their own.
    xtal_hi_rdwr,
    xtal_lo_read,
    xtal_hi_fetch,
    xtal_lo_write,
    xtal_lo_wait,
    DELAYS,
};
constexpr uint16_t DELAYS_NS[] = {
        30,  // xtal_hi_ns: 42 ns
        32,  // xtal_lo_ns: 42 ns
        30,  // xtal_lo_addr: 42 ns
        20,  // xtal_hi_ale: 42 ns
        25,  // xtal_lo_cntl: 42 ns
        16,  // xtal_hi_cntl: 42 ns
        8,   // xtal_lo_inject: 42 ns
        17,  // xtal_lo_output: 42 ns
        8,   // xtal_hi_capture: 42 ns
        24,  // xtal_hi_output: 42 ns
        20,  // xtal_lo_data: 42 ns
        8,   // xtal_hi_input: 42 ns
        20,  // xtal_hi_rdwr: 42 ns
        8,   // xtal_lo_read: 42 ns
        25,  // xtal_hi_fetch: 42 ns
        8,   // xtal_lo_write: 42 ns
        20,  // xtal_lo_wait: 42 ns
};


inline void assert_int0() {
    digitalWriteFast(PIN_INT0, LOW);
}

inline void negate_int0() {
    digitalWriteFast(PIN_INT0, HIGH);
}

inline void assert_int1() {
    digitalWriteFast(PIN_INT1, LOW);
}

inline void negate_int1() {
    digitalWriteFast(PIN_INT1, HIGH);
}

inline auto signal_ale() {
    return digitalReadFast(PIN_ALE);
}

inline auto signal_psen() {
    return digitalReadFast(PIN_PSEN);
}

inline auto signal_rd() {
    return digitalReadFast(PIN_RD);
}

inline auto signal_wr() {
    return digitalReadFast(PIN_WR);
}

inline auto signal_xtype() {
    return digitalReadFast(PIN_XTYPE);
}

void negate_reset() {
    digitalWriteFast(PIN_RST, LOW);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_EA,
        PIN_T0,
        PIN_T1,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_RST,
        PIN_INT0,
        PIN_INT1,
};

constexpr uint8_t PINS_PULLUP[] = {
        PIN_XTAL,
        PIN_XTYPE,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_AD0,
        PIN_AD1,
        PIN_AD2,
        PIN_AD3,
        PIN_AD4,
        PIN_AD5,
        PIN_AD6,
        PIN_AD7,
        PIN_AD8,
        PIN_AD9,
        PIN_AD10,
        PIN_AD11,
        PIN_AD12,
        PIN_AD13,
        PIN_AD14,
        PIN_AD15,
        PIN_ALE,
        PIN_PSEN,
        PIN_RD,
        PIN_WR,
        PIN_P10,
        PIN_P11,
        PIN_P12,
        PIN_P13,
        PIN_P14,
        PIN_P15,
        PIN_P16,
        PIN_P17,
};

}  // namespace

template <ClockType E>
struct PinsI8051::Traits {
    // PIN_XTAL's level for XTAL2 high: the P80C51 takes it on XTAL1.
    static constexpr uint8_t xtalHi = E == CLK_XTAL2 ? HIGH : LOW;
    static constexpr uint8_t xtalLo = E == CLK_XTAL2 ? LOW : HIGH;
};

// Calls |f| with the engine as a compile-time constant.
template <typename F>
auto PinsI8051::withEngine(F f) {
    switch (_engine) {
    case CLK_XTAL1:
        return f(std::integral_constant<ClockType, CLK_XTAL1>());
    default:
        return f(std::integral_constant<ClockType, CLK_XTAL2>());
    }
}

template <uint8_t D>
inline void PinsI8051::delay() const {
    if constexpr (DELAYS_NS[D] != 0)
        delayNanoseconds(DELAYS_NS[D]);
}


template <ClockType E>
inline void PinsI8051::xtal_hi() const {
    digitalWriteFast(PIN_XTAL, Traits<E>::xtalHi);
}

template <ClockType E>
inline void PinsI8051::xtal_lo() const {
    digitalWriteFast(PIN_XTAL, Traits<E>::xtalLo);
}

PinsI8051::PinsI8051() {
    auto regs = new RegsI8051(this);
    _regs = regs;
    _devs = new DevsI8051();
    _data = new DataI8051(regs, _devs);
    _mems = new ProgI8051(_data);
}

PinsI8051::~PinsI8051() {
    delete _data;
}

template <ClockType E>
inline void PinsI8051::xtal_cycle_lo() const {
    xtal_hi<E>();
    delay<xtal_hi_ns>();
    xtal_lo<E>();
}

template <ClockType E>
inline void PinsI8051::xtal_cycle() const {
    xtal_hi<E>();
    delay<xtal_hi_ns>();
    xtal_lo<E>();
    delay<xtal_lo_ns>();
}

void PinsI8051::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    pinMode(PIN_XTAL, OUTPUT);
    digitalWriteFast(PIN_XTAL, HIGH);
    delayNanoseconds(1000);
    // P8051: XTAL=XTAL2, XTAL_TYPE=XTAL1=GND
    // P80C51: XTAL=XTAL1, XTAL_TYPE=XTAL1=XTAL
    _engine = signal_xtype() == LOW ? CLK_XTAL2 : CLK_XTAL1;
    withEngine([&](auto e) { resetCpu<decltype(e)::value>(); });
}

template <ClockType E>
void PinsI8051::resetCpu() {
    xtal_lo<E>();
    delay<xtal_lo_ns>();

    // A reset is accomplished by holding the RST pin high for at
    // least two machine cycles (24 oscillator periods).
    for (auto i = 0; i < 30; ++i)
        xtal_cycle<E>();
    negate_reset();
    Cycles::reset();
    _regs->save();
}

// ALE-high oscillator periods taken for idle mode.
constexpr auto idle_cycles = 1000;

template <ClockType E>
Signals *PinsI8051::prepareCycle() {
    auto s = Signals::put();
    // #PSEN:S1H2, #RD/#WR:S4H2
    // ALE is high a few oscillator periods a cycle; it stays high only in
    // an 80C51's idle mode, which an interrupt or a reset ends: reset it
    // rather than wait for ever (in power-down ALE stays low instead).
    for (auto n = 0;; ++n) {
        // #PSEN:S2L1/S2L2, #RD/#WR:S5L1/S5L2
        xtal_lo<E>();  // S2L2/S5L2 triggers ALE-
        delay<xtal_lo_addr>();
        s->getAddress();
        // #PSEN:S2H1/S2H2, #RD/#WR:S5H1/S5H2
        xtal_hi<E>();
        delay<xtal_hi_ale>();
        if (signal_ale() == LOW)
            break;
        if (n >= idle_cycles) {
            if (_idleReset) {
                // The reset's own cycles found none: no CPU is running.
                _idleFailed = true;
                s->noCycle();
                return s;
            }
            cli.println("?halt: idle, reset");
            _idleReset = true;
            _idleFailed = false;
            resetPins();
            // save() left the PC where its own code ended; go back to 0000.
            _regs->restore();
            _idleReset = false;
            if (_idleFailed)
                cli.println("?halt: no bus cycle");
            // End the run here: the instruction this cycle belonged to is
            // gone, and the CPU waits at 0000 for the next one.
            _idleHalt = true;
            s->noCycle();
            return s;
        }
    }
    // #PSEN:S3L1, #RD/#WR:S6L1
    xtal_lo<E>();  // S3L1/S6L1 triggers #PSEN-
    delay<xtal_lo_cntl>();
    // #PSEN:S3H1, #RD/#WR:S6H1
    xtal_hi<E>();
    delay<xtal_hi_cntl>();
    s->getControl();  // read #PSEN=L
    if (s->fetch()) {
        // #PSEN:S3H1
        return s;
    }
    // #RD/#WR:S6L2
    xtal_lo<E>();
    delay<xtal_lo_ns>();
    // #RD/#WR:S6H2/S1L1
    xtal_cycle<E>();  // S1L1 triggers #RD/WR-
    // #RD/#WR:S1H1
    xtal_hi<E>();
    delay<xtal_hi_rdwr>();
    s->getControl();  // read #RD/#WR, which fall with this edge
    // #RD/#WR:S1H1
    return s;
}

template <ClockType E>
Signals *PinsI8051::completeCycle(Signals *s) {
    // #PSEN:S3H1
    auto d = devs<DevsI8051>();
    if (s->fetch()) {  // program read
        // S3L2
        xtal_lo<E>();
        if (s->readMemory()) {
            s->data = mems<ProgI8051>()->read_byte(s->addr);
            delay<xtal_lo_read>();
        } else {
            delay<xtal_lo_inject>();
        }
        // S3H2
        xtal_hi<E>();
        s->setData();
        delay<xtal_hi_output>();
        // S1L1
        xtal_lo<E>();
        Signals::outputMode();
        delay<xtal_lo_output>();
        // S1H1
        xtal_hi<E>();
        d->uartLoop();
        delay<xtal_hi_fetch>();
        // S1L2
        xtal_lo<E>();  // S1L2 triggers #PSEN+ and ALE+
        Cycles::next();
        // S1H2
        xtal_hi<E>();
        Signals::inputMode();
        // ALE=H
        return s;
    }

    // #RD/#WR:S1H1
    if (s->read()) {  // external data read
        // S1L2
        xtal_lo<E>();
        if (s->readMemory()) {
            s->data = _data->read(s->addr);
            delay<xtal_lo_read>();
        } else {
            delay<xtal_lo_inject>();
        }
        // S2H1
        xtal_hi<E>();
        s->setData();
        delay<xtal_hi_output>();
        // S2L2
        xtal_lo<E>();
        Signals::outputMode();
        delay<xtal_lo_output>();
    } else {  // external data write
        // S1L2
        xtal_lo<E>();
        s->getData();
        delay<xtal_lo_write>();
        //  S1H2
        xtal_hi<E>();
        if (s->writeMemory()) {
            _data->write(s->addr, s->data);
        } else {
            delay<xtal_hi_capture>();
        }
        // S2L1
        xtal_lo<E>();
        delay<xtal_lo_ns>();
    }
    // S2H1/S2L2
    xtal_cycle<E>();
    // S2H2/S3L1
    xtal_cycle_lo<E>();
    d->loop();
    d->uartLoop();
    delay<xtal_lo_wait>();
    // S3H1/S3L2
    xtal_cycle_lo<E>();
    d->uartLoop();
    delay<xtal_lo_wait>();
    // S3H2/S1L1
    xtal_cycle_lo<E>();  // S1L1 triggers #RD/#WR+
    Cycles::next();
    delay<xtal_lo_wait>();
    // S1H1
    xtal_hi<E>();
    Signals::inputMode();
    delay<xtal_hi_input>();
    // S1L2
    xtal_lo<E>();  // S1L2 triggers ALE+
    delay<xtal_lo_data>();
    // S1H2
    xtal_hi<E>();
    // ALE=H
    return s;
}

template <ClockType E>
Signals *PinsI8051::inject(uint8_t data) {
    return completeCycle<E>(prepareCycle<E>()->inject(data));
}

template <ClockType E>
void PinsI8051::inject(const uint8_t data[], uint8_t len) {
    for (auto i = 0; i < len; ++i)
        Cycles::discard(inject<E>(data[i]));
}

void PinsI8051::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

uint8_t PinsI8051::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    return execute(inst, len, addr, buf, max);
}

uint8_t PinsI8051::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    return withEngine([&](auto e) {
        return execute<decltype(e)::value>(inst, len, addr, buf, max);
    });
}

template <ClockType E>
uint8_t PinsI8051::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    // Injected code is a few instructions; give up well before that many
    // cycles rather than wait for ever on a CPU that never writes.
    constexpr auto max_cycles = 64;
    uint8_t inj = 0;
    uint8_t cap = 0;
    for (auto n = 0; inj < len || cap < max; ++n) {
        if (n >= max_cycles) {
            cli.println("?halt: no writes");
            break;
        }
        auto s = _parked ? _parked : prepareCycle<E>();
        _parked = nullptr;
        if (inj == 0 && addr)
            *addr = s->addr;
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle<E>(s);
        if (s->fetch() || s->read())
            ++inj;
        if (s->write()) {
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
    }
    return cap;
}

namespace {
// SJMP $ runs without an interrupt this many times before a save: one
// pending at the halt is taken at the first boundary, and one recognized a
// boundary late at the second.
constexpr auto park_jumps = 2;
// Bus cycles for the SJMPs, or an interrupt's discarded fetches, to reach
// the next fetch; far fewer are needed.
constexpr auto park_cycles = 32;

// 0003, 000B, ..., 002B
bool isVector(uint16_t addr) {
    return addr < 0x30 && (addr & 7) == 3;
}
}  // namespace

// The interrupt LCALL pushes to internal RAM, so the bus shows only fetches
// it discards and then one at its vector. SJMP $ is fed to fetches at the
// halt address until it has run park_jumps times, or a vector fetch shows an
// interrupt taken, which blocks its level: the save then starts at the
// handler, with the program's PC stacked. Only the halt address and the next
// get SJMP's bytes, so a vector fetch is never fed one.
void PinsI8051::park() {
    withEngine([&](auto e) { park<decltype(e)::value>(); });
}

template <ClockType E>
void PinsI8051::park() {
    auto s = prepareCycle<E>();
    const auto here = s->addr;
    auto jumps = 0;
    auto fed = false;  // SJMP's offset went in since the last fetch here
    for (auto n = 0; n < park_cycles; ++n) {
        if (s->fetch()) {
            if (s->addr == here) {
                if (fed && ++jumps == park_jumps)
                    break;
                fed = false;
                s->inject(InstI8051::SJMP);
            } else if (s->addr == here + 1U) {
                s->inject(InstI8051::SJMP_HERE);
                fed = true;
            } else if (isVector(s->addr) && s->addr - here > 3U) {
                // not one of SJMP's own dummy fetches
                break;
            }
        }
        completeCycle<E>(s);
        s = prepareCycle<E>();
    }
    _parked = s;
}

void PinsI8051::idle() {
    static constexpr uint8_t JMP_HERE[] = {
            InstI8051::SJMP,
            0,
            InstI8051::SJMP_HERE,
            0,
    };
    withEngine([&](auto e) {
        inject<decltype(e)::value>(JMP_HERE, sizeof(JMP_HERE));
    });
}

#ifdef PROFILE_CYCLES
// For scripts/record-cycles.py: every bus cycle as the chip makes it, no
// cycle table. The 8051 has no trap instruction, so the profile fills
// memory with MOVX @DPTR,A, and the run ends at the first external write
// after its first instruction; the trap's cycles are kept. Gives up well
// before the ring wraps; the debug pin frames the run, to trigger a
// capture on.
template <ClockType E>
void PinsI8051::loop() {
    constexpr auto MAX_CYCLES = 96;
    const auto first = Signals::put();
    assert_debug();
    for (auto n = 0;; ++n) {
        _devs->loop();
        auto s = prepareCycle<E>();
        if (_idleHalt) {
            negate_debug();
            return;
        }
        completeCycle<E>(s);
        // Only MOVX writes, in its third cycle; the pattern may be one.
        if (s->write() && s->prev(2) != first) {
            negate_debug();
            return;
        }
        if (n >= MAX_CYCLES || haltSwitch()) {
            negate_debug();
            disassembleCycles();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            // No instruction boundary to resume from without the table;
            // drop the reset's own cycles, which run() would print.
            resetPins();
            _regs->restore();
            Cycles::reset();
            return;
        }
    }
}
#else
template <ClockType E>
void PinsI8051::loop() {
    const auto devs = this->devs<DevsI8051>();
    while (true) {
        const auto serviced = devs->loop();
        if (!rawStep<E>() || (serviced && haltSwitch())) {
            return;
        }
    }
}
#endif

void PinsI8051::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    _idleHalt = false;
    startRunTimer();
    withEngine([&](auto e) { loop<decltype(e)::value>(); });
    stopRunTimer();
    restoreBreakInsts();
    {
        // Save before printing: a print with no host reading blocks with
        // XTAL stopped, longer than this NMOS part keeps its SFRs.
        Cycles::Hold hold;
        _regs->save();
    }
    disassembleCycles();
}

template <ClockType E>
bool PinsI8051::rawStep() {
    auto s = prepareCycle<E>();
    if (_idleHalt)
        return false;
    const auto inst = mems<ProgI8051>()->read_byte(s->addr);
    const auto cycles = InstI8051::busCycles(inst);
    if (cycles == 0) {
        // The break opcode (A5): feed SJMP $ in its place and stop.
        completeCycle<E>(s->inject(InstI8051::SJMP));
        static constexpr uint8_t JMP_HERE[] = {
                0,
                InstI8051::SJMP_HERE,
                0,
        };
        inject<E>(JMP_HERE, sizeof(JMP_HERE));
        return false;
    }
    completeCycle<E>(s);
    for (auto i = 1; i < cycles; ++i) {
        s = prepareCycle<E>();
        if (_idleHalt)
            return false;
        completeCycle<E>(s)->clearFetch();
    }
    return true;
}

bool PinsI8051::step(bool show) {
    Cycles::reset();
    _regs->restore();
    _idleHalt = false;
    if (show)
        Cycles::reset();
    if (withEngine([&](auto e) { return rawStep<decltype(e)::value>(); })) {
        Cycles::Hold hold;  // keep the step's cycles for printCycles()
        _regs->save();
        if (show)
            printCycles();
        return true;
    }
    return false;
}

void PinsI8051::assertInt(uint8_t name) {
    if (name == INTR_INT0)
        assert_int0();
    if (name == INTR_INT1)
        assert_int1();
}

void PinsI8051::negateInt(uint8_t name) {
    if (name == INTR_INT0)
        negate_int0();
    if (name == INTR_INT1)
        negate_int1();
}

void PinsI8051::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstI8051::UNKNOWN);
}

void PinsI8051::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsI8051::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsI8051::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
#ifdef PROFILE_CYCLES
    // Every cycle; the opcode fetches rawStep() would take from the run's
    // first one print as I.
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        const auto busCycles = InstI8051::busCycles(_mems->read_byte(s->addr));
        if (busCycles == 0)
            break;  // the break instruction, which rawStep() never runs
        s->setMatched(busCycles);
        i += busCycles;
    }
    printCycles();
#else
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            const auto busCycles = InstI8051::busCycles(s->data);
            for (auto j = len; j < busCycles; ++j)
                s->next(j)->print();
            i += busCycles;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
#endif
}

bool PinsI8051::isCmos() const {
    return _engine == CLK_XTAL1;
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

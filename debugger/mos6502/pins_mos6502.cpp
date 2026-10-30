#include "pins_mos6502.h"
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
//  TL0: min 480 ns; PHI0 low
//  TH0: min 460 ns; PHI0 high
// T02+: max  65 ns; PHI0- to PHI2-
// T02+: max  75 ns; PHI0+ to PHI2+
// Tads: max 225 ns; PHI2- to addr
// Trsw: max 225 ns; PHI2- to R/#W
// Tsys: max 350 ns; PHI2- to SYNC+
// Tmds: max 175 ns; PHI2+ to write data
// Tdsu: min 100 ns; read data to PHI2-

constexpr auto phi0_lo_sync = 30;       // 500
constexpr auto phi0_lo_ns = 218;        // 500
constexpr auto phi0_lo_fetch = 90;      // 500
constexpr auto phi0_lo_loop = 30;       // 500
constexpr auto phi0_lo_execute = 30;    // 500
constexpr auto phi0_hi_read_pre = 240;  // 500
constexpr auto phi0_hi_read_post = 50;  // 500
constexpr auto phi0_hi_write = 334;     // 500
constexpr auto phi0_hi_inject = 70;
constexpr auto phi0_hi_capture = 60;

// The CMOS parts run PHI0 free from the PWM, and RDY holds the CPU in
// every bus cycle until the debugger serves it. RDY changes only early in
// PHI2 low: released there, the cycle ends at the next PHI2 fall; asserted
// there, it holds the cycle that fall began. The W65Cxx sample RDY at the
// PHI2 fall, the G65SC02 and R65C02 through phi1 and phi2, so the one rule
// serves all. The 65C02s give the phase on PHI2O; the W65C816S has none, so
// its phase comes from the PWM counter, PHI2 being PHI0 itself.
//
//  tPCS: min  10 ns; RDY to PHI2- (W65Cxx at 5 V)
//  tPCH: min  10 ns; RDY hold from PHI2-
//  tRDS: min 200 ns; RDY setup (R65C02 at 1 MHz)
//  tBAS: max  33 ns; PHI2- to BA (W65C816S)
//  tMDS: max  85 ns; PHI2+ to write data (R65C02 at 3 MHz)
//  tDHW: min  10 ns; write data hold from PHI2-
//  tDSR: min  40 ns; read data to PHI2- (R65C02 at 3 MHz)
// R65C02: PHI2 low max 5 us; G65SC02: PHI0 low max 10 us.
//
// ATTENTION: unbenched until the rev 11 board exists. The rates below are
// starting points for a K sweep, and the R65C02's tRDS edge is unpinned.
// At 12.5 MHz the W65C816S's 40 ns high half has to fit driving and
// releasing D0-D7 by polling the PWM count; lower the rate if reads fail.
// RDY driven high on each cycle keeps a W65Cxx WAI from stopping, so WAI
// runs as a no-op; STP stops PHI2O, and every wait then times out.

// #RES (pin 28) is FlexPWM3_1_B, PHI0's submodule: never analogWrite() it.

// PHI0 by SoftwareType: 0 for the bit-banged NMOS 6502. The owned parts
// are rated G65SC02P-4, R65C02P3, W65C02S6TPG-14 and W65C816S8P-14.
constexpr uint32_t clock_hz[] = {
        0,         // SW_MOS6502
        4054054,   // SW_G65SC02: 150 MHz / 37
        3000000,   // SW_R65C02
        12500000,  // SW_W65C02S: 150 MHz / 12; /11 leaves a 33 ns half
        12500000,  // SW_W65C816
};
constexpr uint32_t clock_min_hz = 200000;
// The W65Cxx take halves of 35 ns.
constexpr auto clock_half_min_ns = 35;
// Teensy cycles in a PHI0 clock at the slowest rate, so the waits below
// hold at any rate.
constexpr uint32_t clock_max_cycles = F_CPU / clock_min_hz;
// RDY asserted after the PHI2 fall the debugger saw: tPCH.
constexpr auto rdy_hold_ns = 20;
// BA after the PHI2 fall: tBAS.
constexpr auto ba_ns = 40;
// Under the PWM, the devices and the halt switch are polled once in 10 us.
constexpr uint32_t poll_cycles = F_CPU / 100000;

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
}

void negate_rdy() {
    pinMode(PIN_RDY, OUTPUT_OPENDRAIN);
    digitalWriteFast(PIN_RDY, LOW);
}

void hold_rdy() {
    digitalWriteFast(PIN_RDY, LOW);
}

void release_rdy() {
    digitalWriteFast(PIN_RDY, HIGH);
}

void assert_reset() {
    digitalWriteFast(PIN_RES, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RES, HIGH);
}

// #RES first: a glitch on PHI0 while it leaves the PWM is harmless.
constexpr uint8_t PINS_LOW[] = {
        PIN_RES,
        PIN_PHI0,
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

PinsMos6502::PinsMos6502() : _clock(clock_half_min_ns, clock_min_hz) {
    _devs = new DevsMos6502();
    auto mems = new MemsMos6502(_devs);
    _mems = mems;
    _regs = new RegsMos6502(this, mems);
}

bool PinsMos6502::native65816() const {
    return _hardType == HW_W65C816 && signal_e() == LOW;
}

bool PinsMos6502::pwmClock() const {
    using Pin = PwmPin<PIN_PHI0>;
    return (Pin::pad() & 0xF) == Pin::mux;
}

// Hand PHI0 to the PWM on a CMOS part. The CPU is held by RDY in the cycle
// the last bit-banged PHI0 fall began.
void PinsMos6502::startClock() {
    if (_softType == SW_MOS6502)
        return;
#ifdef TUNE_CLOCK
    if (_clockInput == 0)
        _clockInput = clock_hz[_softType];
    _clock.setRate(_clockInput);
#else
    _clock.setRate(clock_hz[_softType]);
#endif
    _phi0High = PwmClock<PIN_PHI0>::sm().VAL3;
    hold_rdy();
    pinMode(PIN_RDY, OUTPUT);
    _clock.start();
}

#ifdef TUNE_CLOCK
bool PinsMos6502::setClockInput(uint32_t hz) {
    if (_softType == SW_MOS6502)
        return false;
    // The rate reloads at a period start, so the running clock never
    // glitches.
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _phi0High = PwmClock<PIN_PHI0>::sm().VAL3;
    _clockInput = rate;
    return true;
}
#endif

bool PinsMos6502::phi2() const {
    if (_hardType == HW_W65C816)
        return _clock.count() < _phi0High;
    return digitalReadFast(PIN_PHI2O) != LOW;
}

// Waits up to two clocks at the slowest rate for PHI2 to read |high|.
bool PinsMos6502::waitPhi2(bool high) const {
    const uint32_t start = ARM_DWT_CYCCNT;
    while (phi2() != high) {
        if (ARM_DWT_CYCCNT - start >= 2 * clock_max_cycles)
            return false;
    }
    return true;
}

void PinsMos6502::waitFall() const {
    waitPhi2(true);
    waitPhi2(false);
}

// Waits for the last quarter of a high half on the W65C816S.
void PinsMos6502::waitLateHigh() const {
    const auto late = _phi0High - _phi0High / 4;
    const uint32_t start = ARM_DWT_CYCCNT;
    while (_clock.count() < late) {
        if (ARM_DWT_CYCCNT - start >= 2 * clock_max_cycles)
            return;
    }
}

// Bit-banged, RDY stays high while the CPU follows the edges the debugger
// makes; under the PWM, RDY holds each cycle until it is served instead.
void PinsMos6502::allReady() const {
    if (!pwmClock())
        assert_rdy();
}

void PinsMos6502::notReady() const {
    if (!pwmClock())
        negate_rdy();
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

void PinsMos6502::checkSoftwareType() {
    if (_hardType == HW_W65C816) {
        _softType = SW_W65C816;
        return;
    }

    // Tell the parts apart by where opcodes some of them lack take the bus.
    inject(InstMos6502::JMP);
    inject(0x00);
    inject(0x10);
    auto base = inject(InstMos6502::BRA);  // 4 clock branch
    inject(InstMos6502::NOP);
    inject(InstMos6502::NOP);
    inject(InstMos6502::NOP);
    auto det65 = inject(InstMos6502::JMP);
    inject(0x00);
    inject(0x10);
    // $80 is a two-byte NOP on the NMOS 6502.
    if (det65->addr == base->addr + 3U) {
        _softType = SW_MOS6502;
        return;
    }
    base = inject(InstMos6502::BBR0);  // 6 clock branch
    inject(InstMos6502::NOP);
    inject(InstMos6502::NOP);
    inject(InstMos6502::NOP);
    inject(InstMos6502::NOP);
    inject(InstMos6502::NOP);
    inject(InstMos6502::NOP);
    det65 = inject(InstMos6502::NOP);
    inject(InstMos6502::NOP);
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
    // PHI0 stays a GPIO, the PWM masked, until a CMOS part is found.
    _clock.begin(clock_min_hz);

    checkHardwareType();
    // #RES must be held low for at least two clock cycles.
    for (auto i = 0; i < 10; i++)
        cycle();
    auto s = prepareCycle();
    negate_reset();
    completeCycle(s);
    Cycles::reset();
    const auto reset_vec = _mems->read16(InstMos6502::VECTOR_RESET);
    _mems->write16(InstMos6502::VECTOR_RESET, 0x1000);  // dummy vector
    // When a positive edge is detected, there is an initialization
    // sequence lasting seven clock cycles.
    for (auto i = 0; i < 10; i++) {
        // there may be spurious write
        const auto s = completeCycle(prepareCycle()->capture());
        // Read dummy reset vector
        if (s->vector() && s->addr == InstMos6502::VECTOR_RESET + 1)
            break;
    }

    _regs->save();
    assert_rdy();
    checkSoftwareType();
    negate_rdy();
    startClock();
    _regs->setIp(reset_vec);
    _mems->write16(InstMos6502::VECTOR_RESET, reset_vec);
}

Signals *PinsMos6502::rawPrepareCycle() {
    if (pwmClock())
        return pwmPrepareCycle();
    auto s = Signals::put();
    if (_hardType == HW_W65C816) {
        // E, this cycle's: whether BA carries a bank.
        s->getControl();
        if (s->addr24()) {
            s->getAddr24();
            return s;
        }
    }
    delayNanoseconds(phi0_lo_sync);
    s->getAddr16();
    return s;
}

Signals *PinsMos6502::prepareCycle() {
    if (pwmClock())
        return pwmPrepareCycle();
    delayNanoseconds(phi0_lo_ns);
    return rawPrepareCycle();
}

Signals *PinsMos6502::completeCycle(Signals *s) {
    if (pwmClock())
        return pwmCompleteCycle(s);
    phi0_hi();
    if (s->write()) {
        delayNanoseconds(phi0_hi_write);
        s->getData();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        } else {
            delayNanoseconds(phi0_hi_capture);
        }
        phi0_lo();
    } else {
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            delayNanoseconds(phi0_hi_inject);
        }
        // [W65C816] Delay to avoid bus conflict with bank address of
        // this bus cycle.
        delayNanoseconds(phi0_hi_read_pre);
        s->outData();
        delayNanoseconds(phi0_hi_read_post);
        // [W65C816] Switch bus direction before falling PHI0 to avoid
        // bus conflict with bank address of next bus cycle. The
        // output data are retained by the bus-hold circuit until bank
        // address is on the bus.
        Signals::inputMode();
        phi0_lo();
    }
    Cycles::next();

    return s;
}

// The CPU is held in the cycle a PHI2 fall began: its address is valid by
// the next rise, and the W65C816S's BA rides D0-D7 while PHI2 is low.
Signals *PinsMos6502::pwmPrepareCycle() {
    auto s = Signals::put();
    if (_hardType == HW_W65C816) {
        waitFall();
        delayNanoseconds(ba_ns);
        s->getControl();
        if (s->addr24()) {
            s->getAddr24();
            return s;
        }
    } else {
        waitPhi2(true);
    }
    s->getAddr16();
    return s;
}

// Releases RDY early in a PHI2 low half, so the cycle ends at the next
// fall, and holds the next cycle after it. An interrupt in between would
// let that cycle run unserved.
Signals *PinsMos6502::pwmCompleteCycle(Signals *s) {
    const auto w65c816 = _hardType == HW_W65C816;
    if (s->read()) {
        if (s->readMemory())
            s->data = _mems->read(s->addr);
        s->stageData();
        noInterrupts();
        waitFall();
        release_rdy();
        if (w65c816) {
            // BA owns D0-D7 while PHI2 is low: drive the data in the high
            // half and let go before the fall; the bus holders keep it.
            waitPhi2(true);
            Signals::driveData();
            waitLateHigh();
            Signals::inputMode();
            waitPhi2(false);
        } else {
            Signals::driveData();
            waitFall();
        }
        delayNanoseconds(rdy_hold_ns);
        hold_rdy();
        interrupts();
        Signals::inputMode();
    } else {
        noInterrupts();
        if (w65c816) {
            // Before the fall brings BA back.
            waitPhi2(true);
            waitLateHigh();
            s->getData();
            waitPhi2(false);
        } else {
            // PHI2O falls after PHI2 does; the bus holders keep the data
            // past tDHW.
            waitFall();
            s->getData();
        }
        release_rdy();
        waitFall();
        delayNanoseconds(rdy_hold_ns);
        hold_rdy();
        interrupts();
        if (s->writeMemory())
            _mems->write(s->addr, s->data);
    }
    Cycles::next();
    return s;
}

Signals *PinsMos6502::cycle() {
    return completeCycle(prepareCycle());
}

Signals *PinsMos6502::inject(uint8_t data) {
    return completeCycle(prepareCycle()->inject(data));
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
    allReady();
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        if (!pwmClock())
            delayNanoseconds(phi0_lo_execute);
        completeCycle(s);
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
    notReady();
}

void PinsMos6502::idle() {
    // Under the PWM, RDY holds the CPU on its own.
    if (pwmClock())
        return;
    // CPU is stopping with RDY=L
    delayNanoseconds(370);
    phi0_hi();
    delayNanoseconds(474);
    phi0_lo();
}

bool PinsMos6502::loop() {
    uint32_t polled = ARM_DWT_CYCCNT - poll_cycles;
    while (true) {
        const auto pwm = pwmClock();
        // Bit-banged, the devices' turn is part of PHI0 low.
        const auto poll = !pwm || ARM_DWT_CYCCNT - polled >= poll_cycles;
        if (poll) {
            polled = ARM_DWT_CYCCNT;
            _devs->loop();
        }
        if (!pwm)
            delayNanoseconds(phi0_lo_fetch);
        auto s = rawPrepareCycle();
        if (s->fetch()) {
            const auto inst = _mems->read_byte(s->addr);
            if (inst == InstMos6502::BRK) {
                const auto opr = _mems->read_byte(s->addr + 1);
                if (opr == 0 || isBreakPoint(s->addr))
                    return suspend();
            }
        } else if (!pwm) {
            delayNanoseconds(phi0_lo_loop);
        }
        completeCycle(s);
        if (poll && haltSwitch())
            return suspend();
    }
}

void PinsMos6502::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    allReady();
    startRunTimer();
    const auto stopped = loop();
    stopRunTimer();
    notReady();
    restoreBreakInsts();
    disassembleCycles();
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
bool PinsMos6502::suspend() {
    _frame.len = 0;
    assert_nmi();
    const Signals *first = nullptr;  // the first fed fetch
    uint32_t park = 0;
    auto parked = false;
    const Signals *push = nullptr;  // the first of consecutive writes
    uint8_t writes = 0;
    for (auto n = 0; n < nmi_cycles; ++n) {
        auto s = prepareCycle();
        if (s->fetch()) {
            if (first == nullptr) {
                first = s;
                park = s->addr;
                parked = true;
            } else if (s->addr != park) {
                parked = false;  // an IRQ entry took it elsewhere
            }
        }
        if (parked && s->read() && s->addr - park < 3) {
            const uint8_t JMP_HERE[] = {
                    InstMos6502::JMP, lo(park), hi(park)};
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
            inject(lo(park));
            inject(hi(park));
            return true;
        }
        completeCycle(s);
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

bool PinsMos6502::rawStep() {
    auto s = rawPrepareCycle();
    const auto inst = _mems->read_byte(s->addr);
    if (InstMos6502::isStop(inst))
        return false;
    if (inst == InstMos6502::BRK) {
        const auto opr = _mems->read_byte(s->addr + 1);
        if (opr == 0 || isBreakPoint(s->addr))
            return false;
    }
    allReady();
    completeCycle(s);
    // A pending IRQ the restored P lets in starts its entry before any
    // instruction, and an NMI raised during it would take its vector: let
    // the IRQ fetch its vector first, so the step stops at its handler.
    if (asserted(PIN_IRQ) && !regs<RegsMos6502>()->irqMasked()) {
        for (auto n = 0; n < nmi_cycles; ++n) {
            const auto v = cycle();
            if (v->read() && (v->addr == InstMos6502::VECTOR_IRQ + 1 ||
                                     v->addr == InstMos6502::W65816_IRQ + 1))
                break;
        }
    }
    if (!suspend()) {
        notReady();
        return false;
    }
    notReady();
    return true;
}

bool PinsMos6502::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    if (rawStep()) {
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

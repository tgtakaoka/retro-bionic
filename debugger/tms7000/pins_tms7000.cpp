#include "pins_tms7000.h"
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
//   tW: min  90 ns ; CLKIN high/low width
//   tC: min 400 ns ; CLKOUT cycle time
// tSUA: min  50 ns ; address valid setup for ALATCH-
//  tHA: min  30 ns ; address hold after ALATCH-
// tSUW: min  50 ns ; R/#W setup for ALATCH-
// tdEL: min -10 ns ; CLKOUT+ to #ENABLE-
//  tAD: min 155 ns ; data input valid for #ENABLE+

constexpr auto clkin_hi_ns = 500;
constexpr auto clkin_lo_ns = 500;
constexpr auto cycle_delay = 0;
// for CLK_DIV2
// CLKIN+ to CLKOUT+: typ 132 ns
// CLKIN+ to CLKOUT-: typ  96 ns
constexpr auto div2_hi_ns = 48;        // 100
constexpr auto div2_lo_ns = 58;        // 100
constexpr auto div2_hi_alatch = 40;    // 100
constexpr auto div2_lo_alatch = 74;    // 100
constexpr auto div2_lo_addr = 30;      // 100
constexpr auto div2_lo_dir = 20;       // 100
constexpr auto div2_hi_write = 0;      // 100
constexpr auto div2_hi_internal = 20;  // 100
// for CLK_DIV4
constexpr auto div4_lo_ns = 38;       // 50
constexpr auto div4_hi_serial = 1;    // 50
constexpr auto div4_hi_wait = 20;     // 50
constexpr auto div4_lo_alatch = 10;   // 50
constexpr auto div4_lo_addr = 4;      // 50
constexpr auto div4_lo_inject = 10;   // 50
constexpr auto div4_lo_write = 10;    // 50
constexpr auto div4_hi_capture = 10;  // 50
// for TMS7002
constexpr auto tms7002_hi_alatch = 68;  // 100
constexpr auto tms7002_lo_alatch = 70;  // 100
constexpr auto tms7002_hi_addr = 30;    // 100

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

void div2_hi(DevsTms7000 *devs) {
    clkin_hi();
    devs->serialLoop();
}

void div2_lo() {
    clkin_lo();
}

void cmos_lo(DevsTms7000 *devs) {
    clkin_lo();
    devs->serialLoop();
}

void cmos_hi() {
    clkin_hi();
}

void (*_clk_hi)(DevsTms7000 *devs);
void (*_clk_lo)();
void (*_clk_cycle)(DevsTms7000 *devs);

void clk_hi(DevsTms7000 *devs) {
    _clk_hi(devs);
}
void clk_lo() {
    _clk_lo();
}
void clk_cycle(DevsTms7000 *devs) {
    _clk_cycle(devs);
}

void div2_clk(DevsTms7000 *devs) {
    clk_hi(devs);
    delayNanoseconds(div2_hi_ns);
    clk_lo();
    delayNanoseconds(div2_lo_ns);
}

void div2_alatch(DevsTms7000 *devs) {
    while (true) {
        clk_hi(devs);
        delayNanoseconds(div2_hi_alatch);
        if (signal_alatch() != LOW)
            break;
        clk_lo();
        delayNanoseconds(div2_lo_alatch);
    }
}

Signals *div2Prepare(DevsTms7000 *devs) {
    auto s = Signals::put();
    // ALATCH=H
    clk_lo();
    s->getAddress();
    delayNanoseconds(div2_lo_addr);
    clk_hi(devs);
    delayNanoseconds(div2_hi_ns);
    clk_lo();
    delayNanoseconds(div2_lo_dir);
    s->getDirection();
    return s;
}

void tms7002_alatch(DevsTms7000 *devs) {
    while (signal_alatch() == LOW) {
        clk_hi(devs);
        delayNanoseconds(tms7002_hi_alatch);
        clk_lo();
        delayNanoseconds(tms7002_lo_alatch);
    }
}

Signals *tms7002Prepare(DevsTms7000 *devs) {
    auto s = Signals::put();
    // ALATCH=H
    clk_hi(devs);
    s->getAddress();
    delayNanoseconds(tms7002_hi_addr);
    clk_lo();
    delayNanoseconds(div2_lo_ns);
    clk_hi(devs);
    delayNanoseconds(div2_hi_ns);
    clk_lo();
    delayNanoseconds(div2_lo_dir);
    s->getDirection();
    return s;
}

void PinsTms7000::div2Complete(Signals *s) const {
    auto d = devs<DevsTms7000>();
    if (s->read()) {  // External read
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            ;  // inject
        }
        s->outData();
        clk_hi(d);
        delayNanoseconds(div2_hi_ns);
        clk_lo();
        s->inputMode();
    } else if (s->write()) {  // External write
        clk_hi(d);
        delayNanoseconds(div2_hi_write);
        s->getData();
        clk_lo();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        } else {
            ;  // capture
        }
    } else {  // Internal cycle
        clk_hi(d);
        s->getData();
        delayNanoseconds(div2_hi_internal);
        clk_lo();
    }
}

void div4_hi(DevsTms7000 *devs) {
    static bool serial = false;
    clkin_hi();
    serial = !serial;
    if (serial) {
        devs->serialLoop();
        delayNanoseconds(div4_hi_serial);
    } else {
        delayNanoseconds(div4_hi_wait);
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
    delayNanoseconds(div4_lo_ns);
    div4_cycle_lo(devs);
    delayNanoseconds(div4_lo_ns);
}

void div4_alatch(DevsTms7000 *devs) {
    while (signal_alatch() == LOW) {
        delayNanoseconds(div4_lo_alatch);
        div4_cycle_lo(devs);
    }
}

Signals *div4Prepare(DevsTms7000 *devs) {
    auto s = Signals::put();
    do {
        s->getAddress();
        div4_cycle_lo(devs);
    } while (signal_alatch() != LOW);
    delayNanoseconds(div4_lo_addr);
    div4_cycle_lo(devs);
    delayNanoseconds(div4_lo_ns);
    div4_hi(devs);
    s->getDirection();
    return s;
}

void PinsTms7000::div4Complete(Signals *s) const {
    if (s->read()) {  // External read
        if (s->readMemory()) {
            s->data = _mems->read(s->addr);
        } else {
            delayNanoseconds(div4_lo_inject);
        }
        s->outData();
        div4_lo();
        s->inputMode();
    } else if (s->write()) {  // External write
        div4_lo();
        delayNanoseconds(div4_lo_ns);
        div4_hi(devs<DevsTms7000>());
        s->getData();
        div4_lo();
        if (s->writeMemory()) {
            _mems->write(s->addr, s->data);
        } else {
            delayNanoseconds(div4_hi_capture);
        }
    } else {  // Internal cycle
        s->getData();
        div4_lo();
    }
}

PinsTms7000::PinsTms7000() {
    auto regs = new RegsTms7000(this);
    _regs = regs;
    _devs = new DevsTms7000();
    _mems = new MemsTms7000(regs, _devs);
}

void PinsTms7000::resetPins() {
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    synchronizeClock();

    // Clear IOCNT0 (>0100) to disable interrupts
    cycle();
    // Inject dummy reset vector >8000
    inject(0x00);  // >FFFF
    inject(0x80);  // >FFFE
    // CLKOUT=H
    _regs->save();
    _regs->setIp(_mems->raw_read16(InstTms7000::VEC_RESET));
    checkHardwareType();
}

void PinsTms7000::synchronizeClock() {
    // CLKOUT works only when #RESET=H
    while (signal_clkout() != LOW)
        clkin_cycle();
    while (signal_clkout() == LOW)
        clkin_cycle();
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
            _clk_hi = div2_hi;
            _clk_lo = div2_lo;
        } else {
            // CMOS device
            _clockType = CLK_CMOS;
            _clk_hi = cmos_lo;
            _clk_lo = cmos_hi;
        }
        _clk_cycle = div2_clk;
        _wait_alatch = div2_alatch;
        _prepareCycle = div2Prepare;
        _completeCycle = &PinsTms7000::div2Complete;
    } else {
        // /4 clock option
        clkin_hi();
        delayNanoseconds(clkin_hi_ns);
        _clockType = CLK_DIV4;
        _clk_hi = div4_hi;
        _clk_lo = div4_lo;
        _clk_cycle = div4_clk;
        _wait_alatch = div4_alatch;
        _prepareCycle = div4Prepare;
        _completeCycle = &PinsTms7000::div4Complete;
    }

    // The RESET function is initiated when the #RESET line of the
    // TMS7000 device is held at a logic zero level for at least five
    // clock cycles.
    auto d = devs<DevsTms7000>();
    assert_reset();
    for (auto i = 0; i < 20; i++)
        clk_cycle(d);
    Signals::resetCycles();
    negate_reset();
    clk_cycle(d);

    if (_clockType == CLK_DIV4) {
        wait_alatch();
    } else {
        while (true) {
            clk_hi(d);
            delayNanoseconds(div2_hi_ns);
            if (signal_alatch() != LOW)
                break;
            clk_lo();
            delayNanoseconds(div2_lo_ns);
            if (signal_alatch() != LOW) {
                _wait_alatch = tms7002_alatch;
                _prepareCycle = tms7002Prepare;
                break;
            }
        }
    }
}

void PinsTms7000::checkHardwareType() {
    inject(0x8A);        // LDA @>0080
    inject(hi(0x0080));  // TMS7000/TMS7001 have 128 bytes internal RAM
    inject(lo(0x0080));  // TMS7002 has 256 bytes internal RAM
    const auto reg80 = cycle();
    if (reg80->external()) {
        inject(0x8A);        // LDA @>0110
        inject(hi(0x0110));  // TMS7000 has no Serial peripheral
        inject(lo(0x0110));  // TMS7001 has a Serial peripheral
        const auto port10 = cycle();
        _hardType = port10->external() ? HW_TMS7000 : HW_TMS7001;
    } else {
        _hardType = HW_TMS7002;
    }
    regs<RegsTms7000>()->restoreA();
    if (_hardType != HW_TMS7000)
        devs<DevsTms7000>()->addSerialHandler(_hardType);
}

Signals *PinsTms7000::completeCycle(Signals *s) const {
    (this->*_completeCycle)(s);
    Signals::nextCycle();
    delayNanoseconds(cycle_delay);
    wait_alatch();
    return s;
}

Signals *PinsTms7000::cycle() const {
    return completeCycle(prepareCycle());
}

Signals *PinsTms7000::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

void PinsTms7000::execInst(const uint8_t *inst, uint8_t len, uint8_t extra) {
    execute(inst, len);
    for (uint8_t i = 0; i < extra; ++i)
        cycle();
}

void PinsTms7000::captureWrites(const uint8_t *inst, uint8_t len, uint8_t *buf,
        uint8_t max, uint16_t *addr) {
    execute(inst, len, buf, max, addr);
}

void PinsTms7000::execute(const uint8_t *inst, uint8_t len, uint8_t *buf,
        uint8_t max, uint16_t *addr) {
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->read()) {
            if (inj == 0 && addr)
                *addr = s->addr;
            if (inj < len)
                ++inj;
        } else if (s->write()) {
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
    }
}

void PinsTms7000::idle() {
    auto s = inject(InstTms7000::JMP);
    inject(InstTms7000::JMP_HERE);
    Signals::discard(s);
}

void PinsTms7000::loop() {
    while (true) {
        _devs->loop();
        if (!rawStep() || haltSwitch())
            return;
    }
}

void PinsTms7000::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

bool PinsTms7000::rawStep() {
    auto s = prepareCycle();
    if (s->intack()) {  // Interrupt
    interrupt:
        completeCycle(s);
        cycle();  // lo(vector)
        cycle();  // hi(vector)
        s = prepareCycle();
    }
    const auto opc = _mems->raw_read(s->addr);
    const auto cycles = InstTms7000::busCycles(opc);
    if (opc == InstTms7000::IDLE || cycles == 0) {
        completeCycle(s->inject(InstTms7000::JMP));
        inject(InstTms7000::JMP_HERE);
        Signals::discard(s);
        return false;
    }
    auto fetch = completeCycle(s);
    for (uint8_t i = 1; i < cycles; ++i) {
        s = prepareCycle();
        if (s->intack())
            goto interrupt;
        completeCycle(s);
    }
    fetch->markFetch(true);
    return true;
}

bool PinsTms7000::step(bool show) {
    Signals::resetCycles();
    _regs->restore();
    if (show)
        Signals::resetCycles();
    if (rawStep()) {
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
    _mems->put_inst(addr, InstTms7000::IDLE);
}

void PinsTms7000::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsTms7000::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
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
}

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

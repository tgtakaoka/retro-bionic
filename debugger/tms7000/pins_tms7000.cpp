#include "pins_tms7000.h"
#include "debugger.h"
#include "devs_tms7000.h"
#include "digital_bus.h"
#include "inst_tms7000.h"
#include "mems_tms7000.h"
#include "regs_tms7000.h"
#include "signals_tms7000.h"

namespace debugger {
namespace tms7000 {

struct PinsTms7000 Pins;

// clang-format off
/**
 * TMS7000 bus cycle.
 *             __    __    __    __    __    __    __    __    __    __    __
 *  /4 CLK |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *            |_____|     |_____|     |_____|     |_____|     |_____|     |_____
 *  /2 CLK ___|     |_____|     |_____|     |_____|     |_____|     |_____|
 *            \ ____\_____\     \     \ ____\_____\           \ ____\_____\
 * CLKOUT _____|     |     |_____|_____|     |     |___________|     |     |____
 *             v     v___________v     v     v     v           v     v__________
 *  ALATCH __________|///////////|___________________________________|//////////
 *         ____v_______________________v                       v________________
 * #ENABLE ____|                       |_______________________|
 *         __________v_______________________________________________v__________
 *    R/#W __________|                                               |__________
 *                   v______________             __v___________v     v__________
 *      AD ----------<______AL______>-----------<______________>-----<____AL____
 *         __________v                                               v__________
 *    R/#W __________|_______________________________________________|__________
 *                   v______________         v_________________v     v__________
 *      AD ----------<______AL______>--------<_________________>-----<____AL____
 */
// clang-format on

namespace {

constexpr auto clkin_hi_ns = 500;
constexpr auto clkin_lo_ns = 500;
constexpr auto clk2_hi_ns = 500;
constexpr auto clk2_lo_ns = 500;
constexpr auto clk4_hi_ns = 250;
constexpr auto clk4_lo_ns = 250;

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

inline void clkin_cycle() {
    clkin_hi();
    delayNanoseconds(clkin_hi_ns);
    clkin_lo();
    delayNanoseconds(clkin_lo_ns);
}

void clk2_hi() {
    clkin_hi();
}

void clk2_lo() {
    clkin_lo();
}

void clk4_hi() {
    clkin_lo();
    delayNanoseconds(clk4_hi_ns);
    clkin_hi();
}

void clk4_lo() {
    clkin_lo();
    delayNanoseconds(clk4_lo_ns);
    clkin_hi();
}

}  // namespace

void (*PinsTms7000::_clk_hi)();
void (*PinsTms7000::_clk_lo)();
void (*PinsTms7000::_clk_cycle)();

void PinsTms7000::clk2_cycle() {
    clk2_hi();
    delayNanoseconds(clk2_hi_ns);
    clk2_lo();
    delayNanoseconds(clk2_lo_ns);
}

void PinsTms7000::clk4_cycle() {
    clk4_hi();
    delayNanoseconds(clk4_hi_ns);
    clk4_lo();
    delayNanoseconds(clk4_lo_ns);
}

void PinsTms7000::synchronize_clk() {
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
        _clk_hi = clk2_hi;
        _clk_lo = clk2_lo;
        _clk_cycle = clk2_cycle;
    } else {
        // /4 clock option
        _clk_hi = clk4_hi;
        _clk_lo = clk4_lo;
        _clk_cycle = clk4_cycle;
        clkin_hi();
        delayNanoseconds(clk4_hi_ns);
    }
}

void PinsTms7000::reset() {
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    synchronize_clk();

    // The RESET function is initiated when the #RESET line of the
    // TMS7000 device is held at a logic zero level for at least five
    // clock cycles.
    assert_reset();
    for (auto i = 0; i < 10; i++)
        clk_cycle();
    negate_reset();
    clk_cycle();
    Signals::resetCycles();
    // Clear IOCNT0 (>0100) to disable interrupts
    cycle();
    // Inject dummy reset vector >8000
    inject(0x00);  // >FFFF
    inject(0x80);  // >FFFE
    // CLKOUT=H
    Regs.save();
    Regs.setIp(Memory.raw_read16(InstTms7000::VEC_RESET));
    checkHardwareType();
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
        inject(0x8A);        // LDA @>0119
        inject(hi(0x0119));  // TMS7002 has no internal peripheral at >0119
        inject(lo(0x0119));  // TMS70C02 haas RXBUF at >0119
        const auto port19 = cycle();
        _hardType = port19->external() ? HW_TMS7002 : HW_TMS70C02;
    }
    Regs.restoreA();
}

Signals *PinsTms7000::prepareCycle() const {
    auto s = Signals::put();
    while (signal_alatch() == LOW)
        clk_cycle();
    // CLKOUT=H, ALATCH=H
    clk_hi();
    delayNanoseconds(clk2_hi_ns);
    // CLKOUT=L
    clk_lo();
    s->getAddress();
    delayNanoseconds(clk2_lo_ns);
    // CLKOUT=H
    clk_hi();
    delayNanoseconds(clk2_hi_ns);
    s->getDirection();
    return s;
}

Signals *PinsTms7000::completeCycle(Signals *s) const {
    if (s->read()) {  // External read
        clk_lo();
        if (s->readMemory()) {
            s->data = Memory.read(s->addr);
        } else {
            ;  // inject
        }
        delayNanoseconds(clk2_lo_ns);
        // CLKOUT=L
        clk_hi();
        s->outData();
        delayNanoseconds(clk2_hi_ns);
        clk_lo();
        delayNanoseconds(clk2_lo_ns);
        // CLKOUT=H
        clk_hi();
        s->inputMode();
        delayNanoseconds(clk2_hi_ns);
        clk_lo();
        delayNanoseconds(clk2_lo_ns);
    } else if (s->write()) {  // External write
        clk_lo();
        delayNanoseconds(clk2_lo_ns);
        // CLKOUT=L
        clk_hi();
        s->getData();
        delayNanoseconds(clk2_hi_ns);
        clk_lo();
        if (s->writeMemory()) {
            Memory.write(s->addr, s->data);
        } else {
            ;  // capture
        }
        delayNanoseconds(clk2_lo_ns);
    } else {  // Internal cycle
        clk_lo();
        delayNanoseconds(clk2_lo_ns);
        s->getData();
    }
    Signals::nextCycle();
    while (signal_alatch() == LOW)
        clk_cycle();
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
        Devs.loop();
        if (!rawStep() || haltSwitch())
            return;
    }
}

void PinsTms7000::run() {
    Regs.restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    Regs.save();
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
    const auto opc = Memory.raw_read(s->addr);
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
    Regs.restore();
    if (show)
        Signals::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        Regs.save();
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
    Memory.put_inst(addr, InstTms7000::IDLE);
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
            const auto len = Memory.disassemble(s->addr, 1) - s->addr;
            i += len;
            if (InstTms7000::isBTJxx(s->data))
                s->next(len - 1)->print();
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

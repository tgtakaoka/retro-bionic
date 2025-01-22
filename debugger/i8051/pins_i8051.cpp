#include "pins_i8051.h"
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
constexpr auto xtal_hi_ns = 17;      // 42 ns
constexpr auto xtal_lo_ns = 17;      // 42 ns
constexpr auto xtal_lo_addr = 4;     // 42 ns
constexpr auto xtal_hi_ale = 7;      // 42 ns
constexpr auto xtal_lo_cntl = 10;    // 42 ns
constexpr auto xtal_hi_cntl = 0;     // 42 ns
constexpr auto xtal_lo_inject = 0;   // 42 ns
constexpr auto xtal_lo_output = 0;   // 42 ns
constexpr auto xtal_hi_capture = 1;  // 42 ns
constexpr auto xtal_lo_data = 20;    // 42 ns
constexpr auto xtal_hi_input = 0;    // 42 ns
// delayNanoseconds(0) takes a bit delay than no delayNanoseconds() call.

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

// P8051 external clock
void xtal2_hi() {
    digitalWriteFast(PIN_XTAL, HIGH);
}

void xtal2_lo() {
    digitalWriteFast(PIN_XTAL, LOW);
}

// P80C51 external clock
void xtal1_lo() {
    digitalWriteFast(PIN_XTAL, LOW);
}

void xtal1_hi() {
    digitalWriteFast(PIN_XTAL, HIGH);
}

PinsI8051::PinsI8051() {
    auto regs = new RegsI8051(this);
    _regs = regs;
    _devs = new DevsI8051();
    _data = new DataI8051(_devs);
    _mems = new ProgI8051(regs, _data);
}

PinsI8051::~PinsI8051() {
    delete _data;
}

inline void PinsI8051::xtal_cycle_lo() const {
    xtal_hi();
    delayNanoseconds(xtal_hi_ns);
    xtal_lo();
}

inline void PinsI8051::xtal_cycle() const {
    xtal_hi();
    delayNanoseconds(xtal_hi_ns);
    xtal_lo();
    delayNanoseconds(xtal_lo_ns);
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
    if (signal_xtype() == LOW) {
        // P8051: XTAL=XTAL2, XTAL_TYPE=XTAL1=GND
        _xtal_hi = xtal2_hi;
        _xtal_lo = xtal2_lo;
    } else {
        // P80C51: XTAL=XTAL1, XTAL_TYPE=XTAL1=XTAL
        // XTAL1 is ~XTAL2 of P8051
        _xtal_hi = xtal1_lo;
        _xtal_lo = xtal1_hi;
    }
    pinMode(PIN_XTAL, OUTPUT);
    xtal_lo();

    // A reset is accomplished by holding the RST pin high gfor at
    // least two machine cycles (24 ocillator periods).
    for (auto i = 0; i < 30; ++i)
        xtal_cycle();
    negate_reset();
    Signals::resetCycles();
    _regs->save();
}

Signals *PinsI8051::prepareCycle() {
    auto s = Signals::put();
    // #PSEN:S1H2, #RD/#WR:S4H2
    do {
        // #PSEN:S2L1/S2L2, #RD/#WR:S5L1/S5L2
        xtal_lo();  // S2L2/S5L2 triggers ALE-
        delayNanoseconds(xtal_lo_addr);
        s->getAddress();
        // #PSEN:S2H1/S2H2, #RD/#WR:S5H1/S5H2
        xtal_hi();
        delayNanoseconds(xtal_hi_ale);
    } while (signal_ale() != LOW);
    // #PSEN:S3L1, #RD/#WR:S6L1
    xtal_lo();  // S3L1/S6L1 triggers #PSEN-
    delayNanoseconds(xtal_lo_cntl);
    // #PSEN:S3H1, #RD/#WR:S6H1
    xtal_hi();
    delayNanoseconds(xtal_hi_cntl);
    s->getControl();  // read #PSEN=L
    if (s->fetch()) {
        // #PSEN:S3H1
        return s;
    }
    // #RD/#WR:S6L2
    xtal_lo();
    delayNanoseconds(xtal_lo_ns);
    // #RD/#WR:S6H2/S1L1
    xtal_cycle();  // S1L1 triggers #RD/WR-
    // #RD/#WR:S1H1
    xtal_hi();
    s->getControl();  // read #RD/#WR read
    // #RD/#WR:S1H1
    return s;
}

Signals *PinsI8051::completeCycle(Signals *s) {
    // #PSEN:S3H1
    auto d = devs<DevsI8051>();
    if (s->fetch()) {  // program read
        // S3L2
        xtal_lo();
        if (s->readMemory()) {
            s->data = _mems->read_byte(s->addr);
        } else {
            delayNanoseconds(xtal_lo_inject);
        }
        // S3H2
        xtal_hi();
        s->setData();
        // S1L1
        xtal_lo();
        Signals::outputMode();
        delayNanoseconds(xtal_lo_output);
        // S1H1
        xtal_hi();
        d->uartLoop();
        // S1L2
        xtal_lo();  // S1L2 triggers #PSEN+ and ALE+
        Signals::nextCycle();
        // S1H2
        xtal_hi();
        Signals::inputMode();
        // ALE=H
        return s;
    }

    // #RD/#WR:S1H1
    if (s->read()) {  // external data read
        // S1L2
        xtal_lo();
        if (s->readMemory()) {
            s->data = _data->read(s->addr);
        } else {
            delayNanoseconds(xtal_lo_inject);
        }
        // S2H1
        xtal_hi();
        s->setData();
        // S2L2
        xtal_lo();
        Signals::outputMode();
        delayNanoseconds(xtal_lo_output);
    } else {  // external data write
        // S1L2
        xtal_lo();
        s->getData();
        //  S1H2
        xtal_hi();
        if (s->writeMemory()) {
            _data->write(s->addr, s->data);
        } else {
            delayNanoseconds(xtal_hi_capture);
        }
        // S2L1
        xtal_lo();
        delayNanoseconds(xtal_lo_ns);
    }
    // S2H1/S2L2
    xtal_cycle();
    // S2H2/S3L1
    xtal_cycle_lo();
    _devs->loop();
    d->uartLoop();
    // S3H1/S3L2
    xtal_cycle_lo();
    d->uartLoop();
    // S3H2/S1L1
    xtal_cycle_lo();  // S1L1 triggers #RD/#WR+
    Signals::nextCycle();
    // S1H1
    xtal_hi();
    Signals::inputMode();
    delayNanoseconds(xtal_hi_input);
    // S1L2
    xtal_lo();  // S1L2 triggers ALE+
    delayNanoseconds(xtal_lo_data);
    // S1H2
    xtal_hi();
    // ALE=H
    return s;
}

Signals *PinsI8051::inject(uint8_t data) {
    return completeCycle(prepareCycle()->inject(data));
}

void PinsI8051::inject(const uint8_t data[], uint8_t len) {
    for (auto i = 0; i < len; ++i)
        Signals::discard(inject(data[i]));
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
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (inj == 0 && addr)
            *addr = s->addr;
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->fetch() || s->read())
            ++inj;
        if (s->write()) {
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
    }
    return cap;
}

void PinsI8051::idle() {
    static constexpr uint8_t JMP_HERE[] = {
            InstI8051::SJMP,
            0,
            InstI8051::SJMP_HERE,
            0,
    };
    inject(JMP_HERE, sizeof(JMP_HERE));
}

void PinsI8051::loop() {
    while (true) {
        _devs->loop();
        if (!rawStep() || haltSwitch()) {
            return;
        }
    }
}

void PinsI8051::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
    _regs->save();
}

bool PinsI8051::rawStep() {
    auto s = prepareCycle();
    const auto inst = _mems->read_byte(s->addr);
    const auto cycles = InstI8051::busCycles(inst);
    if (cycles == 0) {
        completeCycle(s->inject(InstI8051::SJMP));
        static constexpr uint8_t JMP_HERE[] = {
                0,
                InstI8051::SJMP_HERE,
                0,
        };
        inject(JMP_HERE, sizeof(JMP_HERE));
        return false;
    }
    completeCycle(s);
    for (auto i = 1; i < cycles; ++i) {
        completeCycle(prepareCycle())->clearFetch();
    }
    return true;
}

bool PinsI8051::step(bool show) {
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
    _mems->put_inst(addr, InstI8051::UNKNOWN);
}

void PinsI8051::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsI8051::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            const auto cycles = InstI8051::busCycles(s->data);
            for (auto i = len; i < cycles; ++i)
                s->next(i)->print();
            i += cycles;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

bool PinsI8051::isCmos() const {
    return _xtal_hi == xtal1_lo;
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#include "pins_tms9900_base.h"
#include "debugger.h"
#include "inst_tms9900.h"
#include "mems_tms9900.h"
#include "regs_tms9900.h"

#define DEBUG(e) e
// #define DEBUG(e)

namespace debugger {
namespace tms9900 {

namespace {

inline void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

inline void negate_ready() {
    digitalWriteFast(PIN_READY, LOW);
}

// Bus cycles between polls of the halt switch in a run: its yield()
// services the USB while the clock is held, which would otherwise stretch
// every cycle. The devices ration their own polling.
constexpr auto halt_poll_cycles = 16;

}  // namespace

void PinsTms9900Base::startClock(uint32_t hz) {
    analogWriteFrequency(PIN_PWM_CLOCK, hz);
    analogWrite(PIN_PWM_CLOCK, 128);  // 50% at the default 8-bit resolution
}

bool PinsTms9900Base::waitEdge(bool rising, uint32_t ns) {
    const uint8_t from = rising ? LOW : HIGH;
    const uint32_t limit = ns * (F_CPU_ACTUAL / 1000000) / 1000;
    const uint32_t start = ARM_DWT_CYCCNT;
    while (digitalReadFast(PIN_TIMING) != from) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    while (digitalReadFast(PIN_TIMING) == from) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    return true;
}

// READY low holds the CPU in the cycle prepared here.
Signals *PinsTms9900Base::pauseCycle() {
    negate_ready();
    return prepareCycle();
}

void PinsTms9900Base::idle() {
    completeCycle(Signals::put());
    prepareCycle();
}

void PinsTms9900Base::loop() {
    auto s = resumeCycle();
    uint_fast8_t poll = 0;
    while (true) {
        completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        if (++poll >= halt_poll_cycles) {
            poll = 0;
            if (haltSwitch()) {
                suspend(s->addr);
                return;
            }
        }
        // A break is an XOP 15 whose vector holds its own address; its
        // fetch is among the last few cycles before the vector read.
        constexpr auto vec_xop15 = InstTms9900::VEC_XOP15;
        if (s->addr == vec_xop15 && _mems->get_prog(vec_xop15) == vec_xop15) {
            for (auto i = 1; i < 6; ++i) {
                const auto xop = s->prev(i);
                if (xop->fetch() &&
                        _mems->get_prog(xop->addr) == InstTms9900::XOP15) {
                    regs<RegsTms9900>()->breakPoint();
                    xop->clearFetch();
                    xop->next()->clearFetch();
                    Cycles::discard(s);
                    return;
                }
            }
        }
    }
}

void PinsTms9900Base::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    loop();
    stopRunTimer();
    // context has been saved in loop()
    restoreBreakInsts();
    disassembleCycles();
}

// Bus cycles to wait for the NMI vector; an instruction takes far fewer.
constexpr auto nmi_cycles = 1024;

// Runs from pc until NMI reaches its vector, then saves there. NMI is
// asserted only after the first opcode fetch, or the step lands on the
// wrong instruction.
void PinsTms9900Base::suspend(uint16_t pc) {
    bool assert_nmi = true;
    auto s = resumeCycle(pc);
    for (auto n = 0;; ++n) {
        if (s->addr == mems<MemsTms9900>()->vec_nmi())
            break;
        if (n >= nmi_cycles) {
            negateInt(tms9900::INTR_NMI);
            cli.println("?halt: no NMI vector");
            return;
        }
        completeCycle(s);
        if (assert_nmi && s->fetch()) {
            assertInt(tms9900::INTR_NMI);
            assert_nmi = false;
        }
        s = prepareCycle();
    }
    negateInt(tms9900::INTR_NMI);
    Cycles::Hold hold;
    _regs->save();
    Cycles::discard(s);
}

bool PinsTms9900Base::rawStep() {
    suspend(_regs->nextIp());
    return true;
}

bool PinsTms9900Base::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    if (rawStep()) {
        if (show)
            printCycles();
        // context has been saved by suspend()
        return true;
    }
    return false;
}

void PinsTms9900Base::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstTms9900::XOP15);
}

void PinsTms9900Base::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsTms9900Base::findBacktraceStart() {
    auto start = static_cast<const Signals *>(
            backtraceStartByFetchCount<Signals>(_lineLimit));
    // On an 8-bit bus both bytes of an instruction word are fetches:
    // begin at the first.
    if (busBytes() == 1 && (start->addr & 1) && start != Signals::get())
        start = start->prev();
    return start;
}

void PinsTms9900Base::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    const auto unit = busBytes();
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            // print bus cycles other than instruction bytes
            for (uint_fast8_t j = 1; j < len / unit; j++) {
                const auto t = s->next(j);
                if (t->addr < s->addr || t->addr >= s->addr + len)
                    t->print();
            }
            i += len / unit;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace tms9900
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

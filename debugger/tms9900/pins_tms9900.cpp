#include "pins_tms9900.h"
#include "debugger.h"
#include "inst_tms9900.h"
#include "mems_tms9900.h"
#include "regs_tms9900.h"
#include "signals_tms9900.h"

namespace debugger {
namespace tms9900 {

namespace {

inline void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

inline void negate_ready() {
    digitalWriteFast(PIN_READY, LOW);
}

}  // namespace

Signals *PinsTms9900::pauseCycle() {
    negate_ready();
    return prepareCycle();
}

void PinsTms9900::idle() {
    completeCycle(Signals::put());
    prepareCycle();
}

void PinsTms9900::loop() {
    auto s = resumeCycle();
    while (true) {
        completeCycle(s);
        _devs->loop();
        s = prepareCycle();
        if (haltSwitch()) {
            suspend(s->addr);
            return;
        }
        constexpr auto vec_xop15 = InstTms9900::VEC_XOP15;
        if (s->addr == vec_xop15 && _mems->raw_read16(vec_xop15) == vec_xop15) {
            for (auto i = 1; i < 6; ++i) {
                const auto xop = s->prev(i);
                if (xop->fetch() &&
                        _mems->get_inst(xop->addr) == InstTms9900::XOP15) {
                    regs<RegsTms9900>()->breakPoint();
                    xop->clearFetch();
                    xop->next()->clearFetch();
                    Signals::discard(s);
                    return;
                }
            }
        }
    }
}

void PinsTms9900::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    // context has been saved in loop()
    restoreBreakInsts();
    disassembleCycles();
}

void PinsTms9900::suspend(uint16_t pc) {
    bool assert_nmi = true;
    auto s = resumeCycle(pc);
    while (true) {
        if (s->addr == mems<MemsTms9900>()->vec_nmi())
            break;
        completeCycle(s);
        if (assert_nmi && s->fetch()) {
            assertInt(tms9900::INTR_NMI);
            assert_nmi = false;
        }
        s = prepareCycle();
    }
    negateInt(tms9900::INTR_NMI);
    _regs->save();
    Signals::discard(s);
}

bool PinsTms9900::rawStep() {
    suspend(_regs->nextIp());
    return true;
}

bool PinsTms9900::step(bool show) {
    Signals::resetCycles();
    _regs->restore();
    if (show)
        Signals::resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        // context has been saved by suspend()
        return true;
    }
    return false;
}

void PinsTms9900::injectReads(const uint8_t *data, uint8_t len) {
    auto s = resumeCycle();
    for (auto i = 0; i < len;) {
        completeCycle(s->inject(data[i]));
        if (s->read()) {
            i++;
            if (is_internal(s->addr) && i < len)
                i++;
        }
        s = (i < len) ? prepareCycle() : pauseCycle();
    }
}

void PinsTms9900::captureCycles(uint8_t *buf, uint8_t len, bool write) {
    uint16_t addr[len];
    auto s = resumeCycle();
    for (auto i = 0; i < len;) {
        completeCycle(s->capture());
        buf[i] = s->data;
        addr[i] = s->addr;
        if (write) {
            if (s->write()) {
                ++i;
                if (is_internal(s->addr) && i < len)
                    addr[i++] = s->addr + 1;
            }
        } else {
            if (s->read()) {
                ++i;
                if (is_internal(s->addr) && i < len)
                    addr[i++] = s->addr + 1;
            }
        }
        s = (i < len) ? prepareCycle() : pauseCycle();
    }
    for (auto i = 0; i < len; ++i) {
        if (is_internal(addr[i]))
            buf[i] = internal_read(addr[i]);
    }
}

void PinsTms9900::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstTms9900::XOP15);
}

void PinsTms9900::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsTms9900::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            // print bus cycles other than instruction bytes
            for (uint_fast8_t j = 1; j < len; j++) {
                const auto t = s->next(j);
                if (t->addr < s->addr || t->addr >= s->addr + len)
                    t->print();
            }
            i += len;
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

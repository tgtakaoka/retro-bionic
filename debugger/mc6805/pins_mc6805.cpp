#include "pins_mc6805.h"
#include "debugger.h"
#include "mems_mc6805.h"
#include "regs_mc6805.h"
#include "signals_mc6805.h"

namespace debugger {
namespace mc6805 {

void PinsMc6805::resetPins() {
    resetCpu();
    // We should certainly inject SWI by pointing external address here.
    _regs->save();
    _regs->setIp(mems<mc6805::MemsMc6805>()->resetVector());
}

Signals *PinsMc6805::cycle() {
   return completeCycle(prepareCycle());
}

Signals *PinsMc6805::inject(uint8_t data) {
    return completeCycle(prepareCycle()->inject(data));
}

void PinsMc6805::injectReads(const uint8_t *inst, uint_fast8_t len,
        uint_fast8_t cycles, bool discard) {
    // inject |inst| then execute extra |cycles|
    auto s = currCycle();
    for (uint_fast8_t inj = 0; inj < len; ++inj) {
        completeCycle(s->inject(inst[inj]));
        if (discard)
            Signals::discard(s);
        s = prepareCycle();
    }
    for (uint_fast8_t inj = 0; inj < cycles; ++inj) {
        completeCycle(s);
        if (discard)
            Signals::discard(s);
        s = prepareCycle();
    }
}

uint16_t PinsMc6805::captureWrites(
        uint8_t *buf, uint_fast8_t len, bool discard) {
    // capture |len| writes
    uint16_t addr = 0;
    auto s = currCycle();
    for (uint_fast8_t cap = 0; cap < len;) {
        completeCycle(s->capture());
        if (s->write()) {
            if (cap == 0)
                addr = s->addr;
            if (cap < len)
                buf[cap++] = s->data;
        }
        if (discard)
            Signals::discard(s);
        s = prepareCycle();
    }
    return addr;
}

void PinsMc6805::loop() {
    const auto vec_swi = mems<MemsMc6805>()->vecSwi();
    while (true) {
        _devs->loop();
        auto s = rawPrepareCycle();
        if (s->addr == vec_swi) {
            auto r = regs<RegsMc6805>();
            if (r->captureContext(s->prev(5))) {
                const auto pc = r->nextIp() - 1;  //  offset SWI
                if (isBreakPoint(pc) || _mems->read16(vec_swi) == vec_swi) {
                    r->setIp(pc);
                    // inject non-internal RAM address
                    completeCycle(s->inject(hi(0x1000)));
                    inject(lo(0x1000));
                    prepareCycle();
                    restoreBreakInsts();
                    Signals::discard(s->prev(7));
                    disassembleCycles();
                    return;
                }
            }
        }
        if (haltSwitch()) {
            restoreBreakInsts();
            s = suspend(s);
            if (is_internal(s->addr)) {
                // Can't inject instruction for context save
                resetCpu();
                _regs->setIp(s->addr);
            } else {
                disassembleCycles();
                _regs->save();
            }
            return;
        }
        completeCycle(s);
    }
}

Signals *PinsMc6805::suspend(Signals *s) {
    if (s == nullptr)
        s = currCycle();
    while (!s->fetch()) {
        completeCycle(s);
        s = prepareCycle();
    }
    return s;
}

void PinsMc6805::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    // CPU is stopped at fetch
    completeCycle(currCycle());
    loop();
}

bool PinsMc6805::rawStep() {
    const auto pc = _regs->nextIp();
    if (is_internal(pc))
        return false;
    auto s = currCycle(pc);
    const auto inst = _mems->get(s->addr);
    if (!_inst->valid(inst) || _inst->isStop(inst))
        return false;
    completeCycle(s);
    suspend(prepareCycle());
    return true;
}

bool PinsMc6805::step(bool show) {
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

void PinsMc6805::assertInt(uint8_t) {
    digitalWriteFast(PIN_IRQ, LOW);
}

void PinsMc6805::negateInt(uint8_t) {
    digitalWriteFast(PIN_IRQ, HIGH);
}

void PinsMc6805::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstMc6805::SWI);
}

void PinsMc6805::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(currCycle());
    for (auto i = 0; i < cycles; i++) {
        g->next(i)->print();
    }
}

void PinsMc6805::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(currCycle());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto nexti = _mems->disassemble(s->addr, 1);
            const auto len = nexti - s->addr;
            for (uint_fast8_t j = 0; j < len; j++) {
                const auto p = s->next(j);
                if (p->addr < s->addr || p->addr >= nexti)
                    p->print();
            }
            i += len;
        } else {
            s->print();
            ++i;
        }
    }
}

}  // namespace mc6805
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

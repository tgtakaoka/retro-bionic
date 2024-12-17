#include "pins_mc6805.h"
#include "debugger.h"
#include "mems_mc6805.h"
#include "regs_mc6805.h"
#include "signals_mc6805.h"

namespace debugger {
namespace mc6805 {

Signals *PinsMc6805::cycle() const {
    return completeCycle(prepareCycle());
}

Signals *PinsMc6805::cycle(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

void PinsMc6805::injectReads(
        const uint8_t *inst, uint8_t len, uint8_t cycles) const {
    // inject |inst| then execute bus cycles until |cycles|
    auto s = currCycle();
    for (auto inj = 0; inj < len; ++inj) {
        completeCycle(s->inject(inst[inj]));
        s = prepareCycle();
    }
    for (auto inj = len; inj < cycles; ++inj) {
        completeCycle(s);
        s = prepareCycle();
    }
}

void PinsMc6805::captureWrites(
        uint8_t *buf, uint8_t len, uint16_t *addr) const {
    // capture |len| writes and suspend
    auto s = currCycle();
    for (auto cap = 0; cap < len;) {
        completeCycle(s);
        if (s->write()) {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (cap < len)
                buf[cap++] = s->data;
        }
        s = prepareCycle();
    }
}

void PinsMc6805::loop() {
    const auto vec_swi = mems<MemsMc6805>()->vecSwi();
    while (true) {
        _devs->loop();
        auto s = rawPrepareCycle();
        if (s->addr == vec_swi) {
            auto r = regs<RegsMc6805>();
            if (r->saveContext(s->prev(5))) {
                const auto swi_vec = _mems->raw_read16(vec_swi);
                const auto pc = r->nextIp() - 1;  //  offset SWI
                if (isBreakPoint(pc) || swi_vec == vec_swi) {
                    r->setIp(pc);
                    restoreBreakInsts();
                    Signals::discard(s->prev(7));
                    disassembleCycles();
                    suspend();
                    return;
                }
            }
        }
        if (haltSwitch()) {
            restoreBreakInsts();
            suspend();
            disassembleCycles();
            _regs->save();
            return;
        }
        completeCycle(s);
    }
}

void PinsMc6805::suspend() const {
    auto s = currCycle();
    while (!s->fetch()) {
        completeCycle(s);
        s = prepareCycle();
    }
}

void PinsMc6805::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    // CPU is stopped at fetch
    completeCycle(currCycle());
    loop();
}

bool PinsMc6805::rawStep() const {
    auto s = currCycle();
    const auto inst = _mems->raw_read(s->addr);
    if (!_inst->valid(inst) || _inst->isStop(inst))
        return false;
    completeCycle(s);
    prepareCycle();
    suspend();
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

void PinsMc6805::disassembleCycles() const {
    const auto g = Signals::get();
    const auto cycles = g->diff(currCycle());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
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

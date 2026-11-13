#include "pins_mc6805.h"
#include "debugger.h"
#include "mems_mc6805.h"
#include "regs_mc6805.h"
#include "signals_mc6805.h"

namespace debugger {
namespace mc6805 {

bool PinsMc6805::swiVector() const {
    return currCycle()->addr == mems<MemsMc6805>()->vecSwi();
}

bool PinsMc6805::checkBreakPoint(Signals *s) {
    auto r = regs<RegsMc6805>();
    if (r->captureContext(s->prev(5))) {
        const auto pc = r->nextIp() - 1;  //  offset SWI
        const auto vec_swi = mems<MemsMc6805>()->vecSwi();
        if (isBreakPoint(pc) || _mems->read16(vec_swi) == vec_swi) {
            r->captureExtra(pc);
            restoreBreakInsts();
            Cycles::discard(s->prev(7));
            disassembleCycles();
            return true;
        }
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
    _mems->put_prog(addr, InstMc6805::SWI);
}

void PinsMc6805::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(currCycle());
    for (auto i = 0u; i < cycles; i++) {
        g->next(i)->print();
    }
}

const SignalsImpl *PinsMc6805::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsMc6805::printBacktrace() {
#ifdef PROFILE_CYCLES
    // Every cycle; L marks the opcode fetches the chip signals on LI.
    printCycles();
#else
    const auto g = Signals::get();
    const auto cycles = g->diff(currCycle());
    const Signals *prefetch = nullptr;
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch() || prefetch) {
            const auto fetch = prefetch ? prefetch : s;
            const auto nexti = _mems->disassemble(fetch->addr, 1);
            const auto len = nexti - fetch->addr - (prefetch ? 1 : 0);
            uint_fast8_t j = prefetch ? 0 : 1;
            prefetch = nullptr;
            while (j < len) {
                const auto p = s->next(j);
                if (p->addr < fetch->addr || p->addr > nexti)
                    p->print();
                if (p->fetch())
                    prefetch = p;
                j++;
            }
            i += len;
        } else {
            s->print();
            ++i;
        }
    }
#endif
}

}  // namespace mc6805
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

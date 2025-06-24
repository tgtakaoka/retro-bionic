#include "pins_tms370.h"
#include "debugger.h"
#include "inst_tms370.h"
#include "signals_tms370.h"

namespace debugger {
namespace tms370 {

void PinsTms370::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstTms370::TRAP15);
}

void PinsTms370::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

void PinsTms370::disassembleCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
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

}  // namespace tms370
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#include "pins_mc6805.h"
#include "debugger.h"
#include "mems_mc6805.h"
#include "regs_mc6805.h"
#include "signals_mc6805.h"

namespace debugger {
namespace mc6805 {

namespace {
// Longest instruction, with margin; no fetch by then means WAIT or STOP.
constexpr auto FETCH_CYCLES = 64;
// Covers STOP's oscillator start-up delay before the interrupt push.
constexpr auto WAKE_CYCLES = 4096;
}  // namespace

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
            Cycles::discard(s);
        s = prepareCycle();
    }
    for (uint_fast8_t inj = 0; inj < cycles; ++inj) {
        completeCycle(s);
        if (discard)
            Cycles::discard(s);
        s = prepareCycle();
    }
}

uint16_t PinsMc6805::captureWrites(
        uint8_t *buf, uint_fast8_t len, bool discard) {
    // capture |len| writes; an SWI pushes its five within its ten cycles,
    // so give up past that rather than wedge the board
    constexpr auto capture_cycles = 32;
    uint16_t addr = 0;
    auto s = currCycle();
    for (uint_fast8_t cap = 0, n = 0; cap < len; ++n) {
        if (n >= capture_cycles) {
            cli.println("?halt: no writes");
            break;
        }
        completeCycle(s->capture());
        if (s->write()) {
            if (cap == 0)
                addr = s->addr;
            if (cap < len)
                buf[cap++] = s->data;
        }
        if (discard)
            Cycles::discard(s);
        s = prepareCycle();
    }
    return addr;
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

#ifdef PROFILE_CYCLES
// For scripts/record-cycles.py: stop at any SWI, keep every cycle of it,
// and give up well before the ring wraps; the debug pin frames the run, to
// trigger a capture on.
void PinsMc6805::loop() {
    constexpr auto MAX_CYCLES = 96;
    const auto vec_swi = mems<MemsMc6805>()->vecSwi();
    const auto r = regs<RegsMc6805>();
    assert_debug();
    for (auto n = 0;; ++n) {
        _devs->loop();
        auto s = rawPrepareCycle();
        if (s->addr == vec_swi && r->captureContext(s->prev(5))) {
            negate_debug();
            r->captureExtra(r->nextIp() - 1);  // offset SWI
            restoreBreakInsts();
            disassembleCycles();
            return;
        }
        if (n >= MAX_CYCLES || haltSwitch()) {
            negate_debug();
            // Before suspend() adds its cycles, which can wrap the ring.
            disassembleCycles();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            restoreBreakInsts();
            s = suspend(s);
            if (is_internal(s->addr)) {
                // Can't inject instruction for context save
                resetCpu();
                _regs->setIp(s->addr);
            } else if (!_woken) {
                _regs->save();
            }
            return;
        }
        completeCycle(s);
        if (s->fetch())
            _lastOpcode = s->data;
    }
}
#else
void PinsMc6805::loop() {
    const auto vec_swi = mems<MemsMc6805>()->vecSwi();
    while (true) {
        _devs->loop();
        auto s = rawPrepareCycle();
        if (s->addr == vec_swi && checkBreakPoint(s))
            return;
        if (haltSwitch()) {
            restoreBreakInsts();
            s = suspend(s);
            if (is_internal(s->addr)) {
                // Can't inject instruction for context save
                resetCpu();
                _regs->setIp(s->addr);
            } else {
                disassembleCycles();
                if (!_woken)
                    _regs->save();
            }
            return;
        }
        completeCycle(s);
        if (s->fetch())
            _lastOpcode = s->data;
    }
}
#endif

Signals *PinsMc6805::suspend(Signals *s) {
    _woken = false;
    if (s == nullptr)
        s = currCycle();
    for (auto n = 0; !s->fetch(); ++n) {
        if (n >= FETCH_CYCLES)
            return wakeUp(s);
        completeCycle(s);
        s = prepareCycle();
    }
    return s;
}

// WAIT and STOP fetch nothing until an interrupt, and both clear the I
// mask: take an IRQ, whose push stands in for save()'s SWI.
Signals *PinsMc6805::wakeUp(Signals *s) {
    completeCycle(s);
    assertInt(0);
    // After STOP the bus clock restarts at another phase against the
    // debugger's; after WAIT it doesn't.
    if (_inst->isStop(_lastOpcode))
        resyncBus(5 * WAKE_CYCLES);
    uint_fast8_t writes = 0;
    for (auto n = 0; writes < 5; ++n) {
        s = prepareCycle();
        if (n >= WAKE_CYCLES) {
            negateInt(0);
            cli.println("?halt: CPU didn't wake");
            return s;
        }
        completeCycle(s->capture());
        writes = s->write() ? writes + 1 : 0;
    }
    s = prepareCycle();
    negateInt(0);
    regs<RegsMc6805>()->captureContext(s->prev(5));
    static constexpr uint8_t VECTOR[] = {0x10, 0x00};
    injectReads(VECTOR, sizeof(VECTOR), 1);  // V:v:n, as in save()
    _woken = true;
    return currCycle();
}

void PinsMc6805::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    // CPU is stopped at fetch
    completeCycle(currCycle());
    startRunTimer();
    loop();
    stopRunTimer();
    // Every way out of loop(), not only a break point, must take the
    // SWIs back out of memory.
    restoreBreakInsts();
}

bool PinsMc6805::rawStep() {
    const auto pc = _regs->nextIp();
    if (is_internal(pc))
        return false;
    auto s = currCycle(pc);
    const auto inst = _mems->get_prog(s->addr);
    if (!_inst->valid(inst))
        return false;
    _lastOpcode = inst;
    completeCycle(s);
    suspend(prepareCycle());
    return true;
}

bool PinsMc6805::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    if (rawStep()) {
        if (show)
            printCycles();
        if (!_woken)
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

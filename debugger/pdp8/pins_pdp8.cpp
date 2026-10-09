#include "pins_pdp8.h"
#include "debugger.h"
#include "inst_pdp8.h"
#include "regs_pdp8.h"
#include "signals_pdp8.h"

#define DEBUG(e)
// #define DEBUG(e) e

namespace debugger {
namespace pdp8 {

namespace {

void assert_intreq() {
    digitalWriteFast(PIN_INTREQ, LOW);
}

void negate_intreq() {
    digitalWriteFast(PIN_INTREQ, HIGH);
}

}  // namespace

void PinsPdp8::injectReads(const uint16_t *data, uint_fast8_t len) const {
    auto s = resumeCycle(07723);  // dummy address
    for (uint_fast8_t i = 0; i < len;) {
        auto n = completeCycle(s->inject(data[i]));
    inject:
        if (s->read()) {
            i++;
            DEBUG(cli.print("@@  injectReads: inject "));
        } else {
            DEBUG(cli.print("@@  injectReads:        "));
        }
        DEBUG(s->print());
        if (n != s) {
            s = n;
            goto inject;
        }
        s = prepareCycle();
    }
}

void PinsPdp8::captureWrites(const uint16_t *data, uint_fast8_t len,
        uint16_t *buf, uint_fast8_t max) const {
    uint_fast8_t inj = 0;
    uint_fast8_t cap = 0;
    uint_fast8_t cyc = 0;
    auto s = resumeCycle(07723);  // dummy address
    while (inj < len || cap < max) {
        if (inj < len)
            s->inject(data[inj]);
        if (cap < max)
            s->capture();
        auto n = completeCycle(s);
        ++cyc;
    capture:
        DEBUG(cli.print("@@ captureWrites: cyc="));
        DEBUG(cli.printDec(cyc, -3));
        if (s->read()) {
            DEBUG(cli.print("len="));
            DEBUG(cli.printDec(len, -3));
            DEBUG(cli.print("inj="));
            DEBUG(cli.printDec(inj, -3));
            if (inj < len) {
                DEBUG(cli.print("  inject "));
                inj++;
            } else {
                DEBUG(cli.print("         "));
            }
        } else {
            DEBUG(cli.print("cap="));
            DEBUG(cli.printDec(cap, -3));
            DEBUG(cli.print("max="));
            DEBUG(cli.printDec(max, -3));
            if (cap < max) {
                buf[cap++] = s->data;
                DEBUG(cli.print(" capture "));
            } else {
                DEBUG(cli.print("         "));
            }
        }
        DEBUG(s->print());
        if (s != n) {
            s = n;
            ++cyc;
            goto capture;
        }
        s = prepareCycle();
        if (cyc >= 20)
            break;
    }
}

void PinsPdp8::idle() {
    // PDP8 can be static.
}

void PinsPdp8::loop() {
    auto s = resumeCycle(_regs->nextIp());
    while (true) {
        if (completeCycle(s) == nullptr) {
            Cycles::Hold hold;
            _regs->save();
            Cycles::discard(s);
            return;
        }
        fetched(s);
        _devs->loop();
        if (haltSwitch()) {
            // A failed halt: keep the registers from the last good save.
            if (!suspend())
                return;
            s = Signals::put();
            Cycles::Hold hold;
            _regs->save();
            Cycles::discard(s);
            return;
        }
        s = prepareCycle();
    }
}

void PinsPdp8::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
}

// Cycles to wait for the next fetch: fetch, defer, autoindex, execute and
// an interrupt's store take far fewer.
constexpr auto fetch_cycles = 16;

bool PinsPdp8::suspend() {
    auto s = prepareCycle();
    for (auto n = 0; !s->fetch(); ++n) {
        if (n >= fetch_cycles) {
            cli.println("?halt: no fetch");
            return false;
        }
        fetched(completeCycle(s));
        s = prepareCycle();
    }
    return true;
}

bool PinsPdp8::rawStep() {
    auto s = resumeCycle(_regs->nextIp());
    if (completeCycle(s) == nullptr) {
        // A HLT, left unexecuted as loop() leaves it.
        Cycles::Hold hold;
        _regs->save();
        Cycles::discard(s);
        return false;
    }
    fetched(s);
    return suspend();
}

bool PinsPdp8::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsPdp8::assertInt(uint8_t) {
    _intWanted = true;
    if (!_intGate)
        assert_intreq();
}

void PinsPdp8::negateInt(uint8_t) {
    _intWanted = false;
    negate_intreq();
}

void PinsPdp8::gateInterrupts(bool gate) {
    _intGate = gate;
    if (gate) {
        negate_intreq();
    } else if (_intWanted) {
        assert_intreq();
    }
}

// The program's own ION or RTF opens the gate; injected ones never get here.
void PinsPdp8::fetched(const Signals *s) {
    constexpr uint16_t ION = 06001;
    constexpr uint16_t RTF = 06005;
    if (_intGate && s != nullptr && s->fetch() && (s->data == ION || s->data == RTF))
        gateInterrupts(false);
}

void PinsPdp8::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstPdp8::HLT);
}

void PinsPdp8::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
    }
}

const SignalsImpl *PinsPdp8::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsPdp8::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
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

}  // namespace pdp8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

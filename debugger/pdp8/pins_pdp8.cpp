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
            DEBUG(cli.print("@@  injecteReads: inject "));
        } else {
            DEBUG(cli.print("@@  injecteReads:        "));
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
            _regs->save();
            Signals::discard(s);
            return;
        }
        _devs->loop();
        if (haltSwitch()) {
            suspend();
            s = Signals::put();
            _regs->save();
            Signals::discard(s);
            return;
        }
        s = prepareCycle();
    }
}

void PinsPdp8::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
}

void PinsPdp8::suspend() {
    auto s = prepareCycle();
    while (!s->fetch()) {
        completeCycle(s);
        s = prepareCycle();
    }
}

bool PinsPdp8::rawStep() {
    auto s = resumeCycle(_regs->nextIp());
    do {
        completeCycle(s);
        s = prepareCycle();
    } while (!s->fetch());
    return true;
}

bool PinsPdp8::step(bool show) {
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

void PinsPdp8::assertInt(uint8_t name) {
    assert_intreq();
}

void PinsPdp8::negateInt(uint8_t name) {
    negate_intreq();
}

void PinsPdp8::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstPdp8::HLT);
}

void PinsPdp8::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        g->next(i)->print();
    }
}

void PinsPdp8::disassembleCycles() const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
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

}  // namespace pdp8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

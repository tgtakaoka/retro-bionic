#ifndef __PINS_MC68HC12_CYCLE_H__
#define __PINS_MC68HC12_CYCLE_H__

// The cycle engine of PinsMc68hc12, included by each board's file to
// instantiate it with its own class.

#include "clock_mc68hc12.h"
#include "debugger.h"
#include "inst_mc68hc12.h"
#include "pins_mc68hc12.h"
#include "regs_mc68hc12.h"
#include "signals_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

// The delays at the rated bus clock, set by the MC68HC912BD32: its
// address and #DBE settle about 24ns after a quiet edge. The code between
// the edges pads each one.
constexpr PinsMc68hc12::Timing TIMING = {40, 25, 40, 25};

template <uint16_t PinsMc68hc12::Timing::*F>
inline void PinsMc68hc12::delay() const {
#ifdef TUNE_CLOCK
    delayNanoseconds(_timing.*F);
#else
    delayNanoseconds(TIMING.*F);
#endif
}

// Once MISC cleared the stretch, E follows EXTAL, so a bus cycle is four
// fixed edges and E is looked at once, to catch STOP. Until then E is
// polled: an external access holds it high for three more cycles.
template <typename BOARD>
Signals *PinsMc68hc12::prepareCycleOf() {
    using namespace clock;
    const auto board = static_cast<const BOARD *>(this);
    auto s = Signals::put();
    if (_held) {
        _held = false;
        *s = _heldSignals;
        return s;
    }
    if (!_stretch) {
        quiet_edge();
        delay<&Timing::addr>();
        s->getAddr();
        const auto free = board->sampleFree();
        active_edge();
        delay<&Timing::extal>();
        quiet_edge();
        delay<&Timing::data>();
        board->classify(s, free);
        if (clock_e() != LOW)
            return s;
        return stopped(s);
    }
    // E low
    quiet_edge();
    delay<&Timing::addr>();
    s->getAddr();
    const auto free = board->sampleFree();
    active_edge();
    if (!wait_e(HIGH)) {
        // No E: STOP, or a free cycle that doesn't show it.
        if (++_noE < BOARD::STOP_CYCLES) {
            quiet_edge();
            delay<&Timing::extal>();
            s->setFree();
            _done = true;
            active_edge();
            return s;
        }
        _noE = 0;
        return stopped(s);
    }
    _noE = 0;
    // E high
    delay<&Timing::extal>();
    quiet_edge();
    delay<&Timing::data>();
    board->classify(s, free);
    if (s->read() && !s->external()) {
        // A stretched read asserts #DBE late; a free or internal one not
        // at all, and ends with E.
        for (auto n = 0; n < 8; ++n) {
            board->pickupData(s);
            s->getStart();
            active_edge();
            if (wait_e(LOW)) {
                _done = true;
                return s;
            }
            quiet_edge();
            delay<&Timing::data>();
            board->classify(s, free);
            if (s->external())
                break;
        }
    }
    return s;
}

template <typename BOARD>
Signals *PinsMc68hc12::completeCycleOf(Signals *s) {
    using namespace clock;
    const auto board = static_cast<const BOARD *>(this);
    if (_done) {
        _done = false;
    } else if (!s->none()) {
        const auto drive = s->external();
        if (drive) {
            if (s->readMemory())
                s->data = readBus(s);
            s->outData();
            delay<&Timing::setup>();
        } else {
            board->pickupData(s);
        }
        s->getStart();
        // A step's #XIRQ: RTI checks for an interrupt by its second queue
        // refill, the first instruction by the E fall that ends it.
        armStep();
        active_edge();
        // A stretched cycle keeps E high; the data stays until E falls.
        for (auto n = 0; _stretch && !wait_e(LOW) && n < 8; ++n) {
            quiet_edge();
            delay<&Timing::data>();
            if (!drive)
                board->pickupData(s);
            s->getStart();
            active_edge();
        }
        if (drive)
            Signals::inputMode();
        if (s->write() && s->writeMemory())
            writeBus(s);
    }
    Cycles::next();
    return s;
}

template <typename BOARD>
const Signals *PinsMc68hc12::loopOf() {
    _writes = 0;
    _stacked.n = 0;
    uint16_t tryHalt = 0;
    while (true) {
        auto s = prepareCycleOf<BOARD>();
        if (s->none())
            return s;
        if (s->external()) {
            if (s->addr == InstMc68hc12::VEC_SWI) {
                // The queue may fetch a breakpoint's SWI long before it runs,
                // so every SWI stops, and the stacked PC tells which it was.
                Capture stacked;
                exception(s, true, &stacked);
                const auto handler = _mems->read16(InstMc68hc12::VEC_SWI);
                if (isBreakPoint(_regs->nextIp()) ||
                        handler == InstMc68hc12::VEC_SWI)
                    return s;
                // The program's own: put back what it stacked, and go on in
                // its handler.
                for (uint_fast8_t i = 0; i < stacked.n; ++i)
                    _mems->write(stacked.addr[i], stacked.data[i]);
                regs<RegsMc68hc12>()->vectored(handler);
                _regs->restore();
                continue;
            }
            if (tryHalt && s->addr == InstMc68hc12::VEC_XIRQ) {
                negate_xirq();
                exception(s, false);
                return s;
            }
        }
        completeCycleOf<BOARD>(s);
        track(s);
        const auto serviced = _devs->loop();
        if (tryHalt == 0) {
            if (serviced && haltSwitch()) {
                assert_xirq();
                tryHalt = 1;
            }
        } else if (++tryHalt >= 5000) {
            cli.println("?halt: no #XIRQ");
            negate_xirq();
            resetPins();
            return Signals::put();
        }
    }
}

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#include "queue_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

// The parts of Signals that don't need the board, so that host tests can
// replay recorded cycles.

uint_fast8_t Signals::bytes() const {
    // #LSTRB is low for the odd lane; equal to A0 it moves both.
    const auto lstrb = (cntl() & S_LSTRB) != 0;
    return lstrb == ((addr & 1) != 0) ? 2 : 1;
}

uint8_t Signals::byteAt(uint16_t a) const {
    return (a & 1) ? data & 0xFF : data >> 8;
}

void Signals::markFetch(uint16_t inst) {
    mark() |= MARK_FETCH;
    _signals[3] = inst;
    _signals[4] = inst >> 8;
}

#if !defined(ARDUINO)
void Signals::set(uint16_t a, uint16_t d, bool rd, uint8_t move, uint8_t start,
        bool dbe, bool lstrb) {
    clear();
    addr = a;
    data = d;
    cntl() = (rd ? S_READ : 0) | (dbe ? 0 : S_DBE) | (lstrb ? S_LSTRB : 0);
    ipipe() = (move & 3) | ((start & 3) << 2);
    mark() = 0;
}
#endif

void Queue::reset() {
    _st1.valid = _st2.valid = _latch.valid = false;
}

void Queue::replay(Signals *begin, uint_fast8_t cycles) {
    reset();
    for (uint_fast8_t i = 0; i < cycles; ++i)
        begin->next(i)->clearMark();
    for (uint_fast8_t i = 0; i < cycles; ++i) {
        const auto s = begin->next(i);
        if (s->none())
            continue;
        // The movement at this cycle's E rise takes the previous cycle's
        // word.
        const auto in = i == 0 ? nullptr : begin->next(i - 1);
        const auto word = [](Signals *t) {
            Word w;
            w.valid = t != nullptr && t->read();
            w.addr = w.valid ? (t->addr & ~1) : 0;
            if (w.valid)
                t->markProgram();
            return w;
        };
        switch (s->move()) {
        case Signals::MOVE_LAT:
            if (!_latch.valid)
                _latch = word(in);
            break;
        case Signals::MOVE_ALD:
            _st2 = _st1;
            _st1 = word(in);
            _latch.valid = false;
            break;
        case Signals::MOVE_ALL:
            _st2 = _st1;
            _st1 = _latch;
            _latch.valid = false;
            break;
        default:
            break;
        }
        // The start at this cycle's E fall begins the next cycle, the
        // opcode in stage 2.
        if (i + 1 >= cycles)
            continue;
        const auto t = begin->next(i + 1);
        switch (s->start()) {
        case Signals::START_EVEN:
        case Signals::START_ODD:
            if (_st2.valid) {
                const auto odd = s->start() == Signals::START_ODD;
                t->markFetch(_st2.addr + (odd ? 1 : 0));
            }
            break;
        case Signals::START_INT:
            t->markInterrupt();
            break;
        default:
            break;
        }
    }
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

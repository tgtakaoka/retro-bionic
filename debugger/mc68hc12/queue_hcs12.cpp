#include "queue_hcs12.h"
#include "inst_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

void QueueHcs12::reset() {
    _st1.valid = _st2.valid = _st3.valid = false;
    _prebyte = false;
}

void QueueHcs12::replay(Signals *begin, uint_fast8_t cycles) {
    reset();
    for (uint_fast8_t i = 0; i < cycles; ++i)
        begin->next(i)->clearMark();
    for (uint_fast8_t i = 0; i < cycles; ++i) {
        const auto s = begin->next(i);
        if (s->none())
            continue;
        // The start at the E rise: this cycle begins the instruction whose
        // opcode heads stage 3.
        switch (s->beforeRise()) {
        case Signals::START_EVEN:
        case Signals::START_ODD:
            if (_st3.valid) {
                const auto odd = s->beforeRise() == Signals::START_ODD;
                const uint16_t start = _st3.addr + (odd ? 1 : 0);
                if (_prebyte && start == _start + 1) {
                    _prebyte = false;  // the page 2 opcode
                    break;
                }
                const uint8_t opc = odd ? _st3.data : _st3.data >> 8;
                s->markFetch(start);
                _start = start;
                _prebyte = opc == InstMc68hc12::PAGE2;
            }
            break;
        case Signals::START_INT:
            s->markInterrupt();
            _prebyte = false;
            break;
        default:
            break;
        }
        // The movement at the E fall takes this cycle's word.
        if (s->beforeFall() == Signals::MOVE_ALD && s->read()) {
            _st3 = _st2;
            _st2 = _st1;
            _st1.addr = s->addr & ~1;
            _st1.data = s->data;
            _st1.valid = true;
            s->markProgram();
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

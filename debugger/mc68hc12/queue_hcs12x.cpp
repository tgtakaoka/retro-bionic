#include "queue_hcs12x.h"
#include "queue_hcs12.h"

namespace debugger {
namespace mc68hc12 {

void QueueHcs12x::replay(Signals *begin, uint_fast8_t cycles) {
    for (uint_fast8_t i = 0; i < cycles; ++i)
        begin->next(i)->setQueue(0, 0);
    for (uint_fast8_t i = 0; i < cycles; ++i) {
        const auto iq = begin->next(i)->iqstat();
        if (i >= START_LAG) {
            uint8_t start = 0;
            if (iq & IQ_INT) {
                start = Signals::START_INT;
            } else if (iq & IQ_SOD) {
                start = Signals::START_ODD;
            } else if (iq & IQ_SEV) {
                start = Signals::START_EVEN;
            }
            const auto s = begin->next(i - START_LAG);
            s->setQueue(start, s->beforeFall());
        }
        if ((iq & IQ_ALD) && i >= ALD_LAG) {
            const auto s = begin->next(i - ALD_LAG);
            s->setQueue(s->beforeRise(), Signals::MOVE_ALD);
        }
    }
    QueueHcs12().replay(begin, cycles);
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

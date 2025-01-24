#include <string.h>

#include "debugger.h"
#include "regs.h"

namespace debugger {

const char *Regs::cpuName() const {
    return Debugger.target().identity();
}

uint_fast8_t Regs::validRegister(const char *word, uint32_t &max) const {
    const RegList *list;
    for (uint_fast8_t n = 0; (list = listRegisters(n)) != nullptr; ++n) {
        for (uint_fast8_t i = 0; i < list->num; ++i) {
            if (strcasecmp(word, list->head[i]) == 0) {
                max = list->max;
                return list->name + i;
            }
        }
    }
    return 0;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

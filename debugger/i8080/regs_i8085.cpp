#include "regs_i8085.h"
#include "debugger.h"
#include "inst_i8085.h"
#include "pins_i8080_base.h"

namespace debugger {
namespace i8085 {

RegsI8085::RegsI8085(i8080::PinsI8080Base *pins) : RegsI8080(pins) {}

const char *RegsI8085::cpu() const {
    return "i8085";
}

bool RegsI8085::ie() const {
    static const uint8_t RIM[] = {
            0x20,  // RIM
            0x77,  // MOV M, A
    };
    uint8_t rim;
    _pins->captureWrites(RIM, sizeof(RIM), &rim, sizeof(rim));
    return (rim & 0x08) != 0;
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

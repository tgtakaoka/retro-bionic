#include "regs_i8085.h"
#include "pins_i8080_base.h"

namespace debugger {
namespace i8085 {

RegsI8085::RegsI8085(i8080::PinsI8080Base *pins) : RegsI8080(pins) {}

const char *RegsI8085::cpu() const {
    return "i8085";
}

void RegsI8085::save() {
    // RIM reads IE but clobbers A, so DI waits until A is pushed
    static const uint8_t PUSH_ALL[] = {
            0xF5,  // PUSH PSW
            0x20,  // RIM
            0xF3,  // DI
            0x77,  // MOV M, A
            0xC7,  // RST 0
            0xC5,  // PUSH B
            0xD5,  // PUSH D
            0xE5,  // PUSH H
    };
    uint8_t buffer[11];
    _sp = _pins->captureWrites(
            PUSH_ALL, sizeof(PUSH_ALL), buffer, sizeof(buffer));
    _sp++;
    _a = buffer[0];
    _psw = buffer[1];
    _ie = (buffer[2] & 0x08) != 0;
    _pc = be16(buffer + 3) - 5;  // offset instructions before RST
    _b = buffer[5];
    _c = buffer[6];
    _d = buffer[7];
    _e = buffer[8];
    _h = buffer[9];
    _l = buffer[10];
}

void RegsI8085::saveContext(uint16_t pc) {
    uint8_t buffer[2];
    const auto sp = _pins->captureWrites(nullptr, 0, buffer, sizeof(buffer));
    save();
    _sp = sp + 1;  // offset TRAP context save
    _pc = pc;
}

// At the TRAP's fetch from 0024H: |pc| and |sp| are what it stacked from
void RegsI8085::saveTrap(uint16_t pc, uint16_t sp) {
    save();
    _sp = sp;
    _pc = pc;
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

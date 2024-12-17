#include "signals_f3850.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_f3850.h"

namespace debugger {
namespace f3850 {

Signals *Signals::getRomc() {
    romc() = busRead(ROMC);
    flags() = 0;
    return this;
}

Signals *Signals::getData() {
    data = busRead(DB);
    return this;
}

void Signals::outData() const {
    busWrite(DB, data);
    busMode(DB, OUTPUT);
}

void Signals::inputMode() {
    busMode(DB, INPUT);
}

void Signals::markRead(uint16_t a) {
    addr = a;
    flags() |= READ;
}

void Signals::markWrite(uint16_t a) {
    addr = a;
    flags() |= WRITE;
}

void Signals::markIoRead(uint8_t a) {
    addr = a;
    flags() |= IO | READ;
}

void Signals::markIoWrite(uint8_t a) {
    addr = a;
    flags() |= IO | WRITE;
}

void Signals::print() const {
    //                              012345678901234567
    static constexpr char line[] = "R C=xx D=xx A=xxxx";
    static auto &buffer = *new CharBuffer(line);
    buffer.hex8(4, romc());
    buffer.hex8(9, data);
    if (flags() & (READ | WRITE)) {
        buffer[0] = (flags() & READ) ? 'R' : 'W';
        if (flags() & IO) {
            buffer[12] = 'I';
            buffer.hex8(14, addr);
            buffer[16] = 0;
        } else {
            buffer[12] = 'A';
            buffer.hex16(14, addr);
        }
    } else {
        buffer[0] = ' ';
        buffer[12] = 0;
    }
    cli.println(buffer);
}

}  // namespace f3850
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

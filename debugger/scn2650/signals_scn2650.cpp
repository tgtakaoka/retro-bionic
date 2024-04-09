#include "signals_scn2650.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_scn2650.h"

namespace debugger {
namespace scn2650 {

void Signals::getAddr() {
    addr = busRead(ADRL) | busRead(ADRM) | busRead(ADRH);
    cntl() = busRead(CNTL);
}

bool Signals::read() const {
    return (cntl() & CNTL_RW) == 0;
}

bool Signals::write() const {
    return (cntl() & CNTL_RW) != 0;
}

bool Signals::io() const {
    return (cntl() & CNTL_MIO) == 0;
}

bool Signals::vector() const {
    return (cntl() & CNTL_INTACK) != 0;
}

void Signals::getData() {
    data = busRead(DBUS);
}

void Signals::outData() {
    busMode(DBUS, OUTPUT);
    busWrite(DBUS, data);
}

void Signals::print() const {
    //                              01234567890123
    static constexpr char line[] = "MR A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    buffer[0] = io() ? (vector() ? 'V' : 'I') : 'M';
    buffer[1] = write() ? 'W' : 'R';
    if (io()) {
        if (addr & 0x2000) {
            buffer[5] = 'E';
            buffer[6] = ':';
            buffer.hex8(7, addr);
        } else {
            buffer[5] = 'N';
            buffer[6] = 'E';
            buffer[7] = ':';
            buffer[8] = (addr & 0x4000) ? 'D' : 'C';
        }
    } else {
        buffer.hex16(5, addr);
    }
    buffer.hex8(12, data);
    cli.println(buffer);
}

}  // namespace scn2650
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

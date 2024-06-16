#include "signals_z80.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_z80.h"

namespace debugger {
namespace z80 {

void Signals::getAddress() {
    addr = busRead(AL) | busRead(AM) | busRead(AH);
}

void Signals::getControl() {
    cntl() = busRead(CNTL);
}

bool Signals::nobus() const {
    return (cntl() & (CNTL_MREQ | CNTL_IORQ)) == (CNTL_MREQ | CNTL_IORQ);
}

bool Signals::m1() const {
    return (cntl() & CNTL_M1) == 0;
}

bool Signals::mreq() const {
    return (cntl() & CNTL_MREQ) == 0;
}

bool Signals::iorq() const {
    return (cntl() & CNTL_IORQ) == 0;
}

bool Signals::read() const {
    return (cntl() & CNTL_RD) == 0;
}

bool Signals::fetch() const {
    return (cntl() & (CNTL_M1 | CNTL_MREQ)) == 0;
}

bool Signals::intack() const {
    return (cntl() & (CNTL_M1 | CNTL_IORQ)) == 0;
}

void Signals::markRead() {
    cntl() &= ~CNTL_RD;
}

void Signals::getData() {
    data = busRead(D);
}

void Signals::outData() const {
    busMode(D, OUTPUT);
    busWrite(D, data);
}

void Signals::inputMode() const {
    busMode(D, INPUT);
}

void Signals::print() const {
    //                              01234567890123
    static constexpr char line[] = "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (mreq()) {
        buffer[0] = fetch() ? 'F' : (read() ? 'R' : 'W');
        buffer[2] = 'A';
    } else {
        buffer[0] = intack() ? 'A' : (read() ? 'R' : 'W');
        buffer[2] = 'I';
    }
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

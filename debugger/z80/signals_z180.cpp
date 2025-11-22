#include "signals_z180.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_z180.h"

namespace debugger {
namespace z180 {

constexpr auto ASEL_DELAY = 5;
constexpr auto NOREQ = CNTL_MREQ | CNTL_IORQ;
constexpr auto NOBUS = CNTL_RD | CNTL_WR | CNTL_M1;

void Signals::getAddr() {
    addr = busRead(AL) | busRead(AM) | busRead(AH);
}

void Signals::getAddrHigh() {
    digitalWriteFast(PIN_ASEL, HIGH);
    delayNanoseconds(ASEL_DELAY);
    addr |= busRead(AE);
    digitalWriteFast(PIN_ASEL, LOW);
}

bool Signals::getControl() {
    return ((cntl() = busRead(CNTL)) & NOREQ) != NOREQ;
}

bool Signals::fetch() const {
    return (cntl() & (CNTL_M1 | CNTL_MREQ)) == 0;
}

bool Signals::mreq() const {
    return (cntl() & CNTL_MREQ) == 0;
}

bool Signals::iorq() const {
    return (cntl() & CNTL_IORQ) == 0;
}

bool Signals::nobus() const {
    return (cntl() & NOBUS) == NOBUS;
}

bool Signals::read() const {
    return (cntl() & CNTL_RD) == 0;
}

bool Signals::write() const {
    return (cntl() & CNTL_WR) == 0;
}

bool Signals::intack() const {
    return (cntl() & (CNTL_IORQ | CNTL_M1)) == 0;
}

void Signals::getData() {
    data = busRead(D);
}

void Signals::outData() const {
    busWrite(D, data);
    busMode(D, OUTPUT);
}

void Signals::inputMode() const {
    busMode(D, INPUT);
}

void Signals::print() const {
    // cli.printDec(pos(), -4);
    // if (read()) {
    //     cli.print(readMemory() ? 'r' : 'i');
    // } else if (write()) {
    //     cli.print(writeMemory() ? 'w' : 'c');
    // } else {
    //     cli.print(' ');
    // }
    // cli.print(' ');
    //                              012345678901234
    static constexpr char line[] = "R A=xxxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (mreq() && !nobus()) {
        buffer[0] = fetch() ? 'F' : (read() ? 'R' : 'W');
        buffer[2] = 'A';
    } else if (iorq()) {
        buffer[0] = intack() ? 'A' : (read() ? 'R' : 'W');
        buffer[2] = 'I';
    } else {
        buffer[0] = ' ';
        buffer[2] = 'A';
    }
    buffer.hex20(4, addr);
    buffer.hex8(12, data);
    cli.println(buffer);
}

}  // namespace z180
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

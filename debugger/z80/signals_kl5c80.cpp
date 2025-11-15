#include "signals_kl5c80.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_kl5c80.h"

namespace debugger {
namespace kl5c80 {

constexpr auto NOBUS = CNTL_EMRD | CNTL_EMWR | CNTL_EIORD | CNTL_EIOWR;

void Signals::getAddr() {
    addr = busRead(AL) | busRead(AM) | busRead(AH);
    digitalWriteFast(PIN_ASEL, HIGH);
}

void Signals::getAddrHigh() {
    addr |= busRead(AE);
    digitalWriteFast(PIN_ASEL, LOW);
}

bool Signals::getControl() {
    return ((cntl() = busRead(CNTL)) & NOBUS) != NOBUS;
}

bool Signals::fetch() const {
    return (cntl() & (CNTL_M1 | CNTL_EMRD)) == 0;
}

bool Signals::mrd() const {
    return (cntl() & CNTL_EMRD) == 0;
}

bool Signals::mwr() const {
    return (cntl() & CNTL_EMWR) == 0;
}

bool Signals::iord() const {
    return (cntl() & CNTL_EIORD) == 0;
}

bool Signals::iowr() const {
    return (cntl() & CNTL_EIOWR) == 0;
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
    //                              012345678901234
    static constexpr char line[] = "R A=xxxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (mrd()) {
        buffer[0] = fetch() ? 'F' : 'R';
        buffer[2] = 'A';
    } else if (mwr()) {
        buffer[0] = 'W';
        buffer[2] = 'A';
    } else if (iord()) {
        buffer[0] = 'R';
        buffer[2] = 'I';
    } else if (iowr()) {
        buffer[0] = 'W';
        buffer[2] = 'I';
    }
    buffer.hex20(4, addr);
    buffer.hex8(12, data);
    cli.println(buffer);
}

}  // namespace kl5c80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

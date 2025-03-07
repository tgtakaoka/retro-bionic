#include "signals_i8085.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_i8085.h"

namespace debugger {
namespace i8085 {

void Signals::getAddress() {
    addr = busRead(ADDR);
}

void Signals::getStatus() {
    iom() = digitalReadFast(PIN_IOM);
    cntl() = busRead(CNTL);
}

void Signals::getDirection() {
    cntl() = busRead(CNTL);
}

bool Signals::read() const {
    return (cntl() & CNTL_S) == S_READ;
}

bool Signals::write() const {
    return (cntl() & CNTL_S) == S_WRITE;
}

bool Signals::fetch() const {
    return (cntl() & CNTL_S) == S_FETCH;
}

bool Signals::readEnable() const {
    return (cntl() & CNTL_RD) == 0;
}

bool Signals::writeEnable() const {
    return (cntl() & CNTL_WR) == 0;
}

bool Signals::intAck() const {
    return (cntl() & CNTL_INTA) == 0;
}

bool Signals::halt() const {
    return (cntl() & CNTL_S) == S_HALT;
}

void Signals::getData() {
    data = busRead(AD);
}

void Signals::outData() const {
    busWrite(AD, data);
    busMode(AD, OUTPUT);
}

void Signals::inputMode() {
    busMode(AD, INPUT);
}

void Signals::print() const {
    // cli.printDec(pos(), -4);
    //                              01234567890123
    static constexpr char line[] = "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (readEnable()) {
        buffer[0] = 'R';
    } else if (writeEnable()) {
        buffer[0] = 'W';
    } else {
        buffer[0] = ' ';
    }
    if (memory()) {
        if (fetch())
            buffer[0] = 'F';
        buffer[2] = 'A';
    } else {
        if (intAck())
            buffer[0] = 'A';
        buffer[2] = 'I';
    }
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

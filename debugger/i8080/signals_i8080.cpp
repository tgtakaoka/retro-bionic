#include "signals_i8080.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_i8080.h"

namespace debugger {
namespace i8080 {

void Signals::getAddress() {
    addr = busRead(ADRL) | busRead(ADRM) | busRead(ADRH);
}

void Signals::getStatus() {
    status() = busRead(DATA);
}

bool Signals::memory() const {
    const auto NON_MEM = S_INTA | S_HLTA | S_OUT | S_INP;
    return (status() & NON_MEM) == 0;
}

bool Signals::read() const {
    const auto READ = S_MEMR | S_INP | S_INTA;
    return (status() & READ) != 0;
}

bool Signals::write() const {
    return (status() & S_READ) == 0;
}

bool Signals::ioRead() const {
    return (status() & S_INP) != 0;
}

bool Signals::ioWrite() const {
    return (status() & S_OUT) != 0;
}

bool Signals::intAck() const {
    return (status() & S_INTA) != 0;
}

bool Signals::fetch() const {
    return (status() & S_M1) != 0;
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void Signals::inputMode() {
    busMode(DATA, INPUT);
}

void Signals::print() const {
    // cli.printDec(pos(), -4);
    //                              01234567890123
    static constexpr char line[] = "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (read()) {
        if (intAck()) {
            buffer[0] = 'A';
        } else if (fetch()) {
            buffer[0] = 'F';
        } else {
            buffer[0] = 'R';
        }
    } else if (write()) {
        buffer[0] = 'W';
    } else {
        buffer[0] = ' ';
    }
    if (ioRead() || ioWrite() || intAck()) {
        buffer[2] = 'I';
    } else {
        buffer[2] = 'A';
    }
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

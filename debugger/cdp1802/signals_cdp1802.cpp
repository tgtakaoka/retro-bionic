#include "signals_cdp1802.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_cdp1802.h"

namespace debugger {
namespace cdp1802 {

void Signals::getStatus() {
    cntl() = busRead(CNTL);
}

void Signals::getAddr1() {
    addr = static_cast<uint16_t>(busRead(MA)) << 8;
}

void Signals::getAddr2() {
    addr |= busRead(MA);
}

uint_fast8_t Signals::getIoAddr() {
    return nBus() = busRead(N);
}

void Signals::getDirection() {
    cntl() = busRead(CNTL);
}

void Signals::getData() {
    data = busRead(DBUS);
}

void Signals::outData() const {
    busWrite(DBUS, data);
    busMode(DBUS, OUTPUT);
}

void Signals::inputMode() {
    busMode(DBUS, INPUT);
}

bool Signals::read() const {
    return (cntl() & CNTL_MRD) == 0;
}

bool Signals::write() const {
    return (cntl() & CNTL_MWR) == 0;
}

bool Signals::fetch() const {
    return sc() == 0;
}

bool Signals::vector() const {
    return sc() == 3;
}

Signals::State Signals::sc() const {
    return State(cntl() & CNTL_SC);
}

void Signals::print() const {
    //                              012345678901234567
    static constexpr char line[] = "LW A=xxxx D=xx N=x";
    static auto &buffer = *new CharBuffer(line);
    static constexpr char SC[] = {'F', ' ', 'D', 'I'};
    buffer[0] = SC[sc()];
    buffer[1] = read() ? 'R' : (write() ? 'W' : '-');
    buffer.hex16(5, addr);
    buffer.hex8(12, data);
    if (ioAddr()) {
        buffer[14] = ' ';
        buffer.hex4(17, ioAddr());
    } else {
        buffer[14] = 0;
    }
    cli.println(buffer);
}

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

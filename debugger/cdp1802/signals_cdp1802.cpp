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

void Signals::getDirection() {
    cntl() = busRead(CNTL);
}

void Signals::getData() {
    data = busRead(DBUS);
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

uint8_t Signals::sc() const {
    return cntl() & CNTL_SC;
}

void Signals::print() const {
    //                              01234567890123
    static constexpr char line[] = "LW A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    static constexpr char SC[] = {'F', ' ', 'D', 'I'};
    buffer[0] = SC[sc()];
    buffer[1] = read() ? 'R' : (write() ? 'W' : '-');
    buffer.hex16(5, addr);
    buffer.hex8(12, data);
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

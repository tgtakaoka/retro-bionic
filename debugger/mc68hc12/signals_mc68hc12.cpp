#include "signals_mc68hc12.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

void Signals::getAddr() {
    addr = busRead(AD);
    const auto c = busRead(CNTL);
    cntl() = (c & CNTL_RW) ? S_READ : 0;
    if (digitalReadFast(PIN_LSTRB) != LOW)
        cntl() |= S_LSTRB;
    ipipe() = (c >> CNTL_IPIPE_gp) & 3;
    mark() = 0;
}

void Signals::getDbe() {
    if (digitalReadFast(PIN_DBE) != LOW)
        cntl() |= S_DBE;
}

void Signals::getStart() {
    ipipe() = (ipipe() & 3) | (((busRead(CNTL) >> CNTL_IPIPE_gp) & 3) << 2);
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
    //                              0123456789012345
    static constexpr char line[] = "  A=xxxx D=xx";
    static constexpr char word[] = "  A=xxxx D=xxxx";
    auto &buffer = Cycles::buffer();
    if (none()) {
        buffer.set("  -");
        cli.println(buffer);
        return;
    }
    const auto two = bytes() == 2;
    buffer.set(two ? word : line);
    if (write()) {
        buffer[0] = 'W';
    } else if (!external()) {
        buffer[0] = 'r';  // free or internal: not on the external bus
    } else if (program()) {
        buffer[0] = 'P';
    } else {
        buffer[0] = 'R';
    }
    if (interrupt())
        buffer[1] = '!';
    buffer.hex16(4, addr);
    if (two) {
        buffer.hex16(11, data);
    } else {
        buffer.hex8(11, byteAt(addr));
    }
    cli.println(buffer);
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

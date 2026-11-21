#include "signals_kl5c8400.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_kl5c8400.h"

namespace debugger {
namespace kl5c8400 {

constexpr auto NOBUS = CNTL_MRD | CNTL_MWR | CNTL_IORD | CNTL_IOWR;

void Signals::getAddress() {
    addr = busRead(AL) | busRead(AM) | busRead(AH);
}

bool Signals::getControl() {
    return ((cntl() = busRead(CNTL)) & NOBUS) != NOBUS;
}

bool Signals::busy() {
    return (busRead(CNTL) & NOBUS) != NOBUS;
}

void Signals::noCycle() {
    cntl() = NOBUS;
}

bool Signals::nobus() const {
    return (cntl() & NOBUS) == NOBUS;
}

bool Signals::fetch() const {
    // In KC80 mode #M1 stays high through an opcode fetch and goes low in
    // the other memory and I/O cycles.
    return (cntl() & (CNTL_M1 | CNTL_MRD)) == CNTL_M1;
}

bool Signals::mrd() const {
    return (cntl() & CNTL_MRD) == 0;
}

bool Signals::mwr() const {
    return (cntl() & CNTL_MWR) == 0;
}

bool Signals::iord() const {
    return (cntl() & CNTL_IORD) == 0;
}

bool Signals::iowr() const {
    return (cntl() & CNTL_IOWR) == 0;
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
    //                              01234567890123
    static constexpr char line[] = "R A=xxxx D=xx";
    auto &buffer = Cycles::buffer();
    buffer.set(line);
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
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace kl5c8400
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

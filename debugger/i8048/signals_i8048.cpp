#include "signals_i8048.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_i8048.h"

namespace debugger {
namespace i8048 {

void Signals::getAddress() {
    addr = busRead(ADDR);
}

bool Signals::getControl() {
    cntl() = busRead(CNTL);
    return valid();
}

void Signals::getData() {
    data = busRead(DB);
}

bool Signals::read() const {
    return (cntl() & CNTL_RD) == 0;
}

bool Signals::write() const {
    return (cntl() & CNTL_WR) == 0;
}

bool Signals::fetch() const {
    return (cntl() & CNTL_PSEN) == 0;
}

bool Signals::port() const {
    return (cntl() & CNTL_PROG) == 0;
}

bool Signals::valid() const {
    return cntl() != (CNTL_RD | CNTL_WR | CNTL_PSEN | CNTL_PROG);
}

void Signals::markInvalid() {
    cntl() = (CNTL_RD | CNTL_WR | CNTL_PSEN | CNTL_PROG);
}

void Signals::clearFetch() {
    cntl() |= CNTL_PSEN;
}

void Signals::print() const {
    //                              012345678901
    static constexpr char line[] = "R A=xxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (port()) {
        static constexpr char CONTROL[] = {'R', 'W', 'O', 'A'};
        buffer[0] = CONTROL[(addr >> 10) & 3];
        buffer[2] = 'P';
        const auto port = ((addr >> 8) & 3) + 4;
        buffer[4] = port + '0';
        buffer.hex8(5, addr & 0xFF);
    } else if (fetch()) {
        buffer[0] = 'P';
        buffer[2] = 'A';
        buffer.hex12(4, addr);
    } else {
        buffer[0] = read() ? 'R' : (write() ? 'W' : ' ');
        buffer[2] = 'X';
        buffer[4] = ' ';
        buffer.hex8(5, addr);
    }
    buffer.hex8(10, data);
    cli.println(buffer);
}

}  // namespace i8048
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

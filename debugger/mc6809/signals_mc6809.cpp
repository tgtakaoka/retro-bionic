#include "signals_mc6809.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc6809_base.h"

namespace debugger {
namespace mc6809 {

void Signals::getHighAddr() {
    addr = busRead(AM) | busRead(AH);
}

void Signals::getLowAddr() {
    addr |= busRead(AL);
}

void Signals::getDirection() {
    cntl() = busRead(CNTL);
}

void Signals::getControl() {
    clearFetch();
}

bool Signals::vector() const {
    return (cntl() & (CNTL_BA | CNTL_BS)) == CNTL_BS;
}

void Signals::getData() {
    data = busRead(D);
}

void Signals::outData() const {
    busWrite(D, data);
    busMode(D, OUTPUT);
}

void Signals::inputMode() {
    busMode(D, INPUT);
}

void Signals::print() const {
    LOG_MATCH(cli.printDec(pos(), 3));
    LOG_MATCH(if (fetch()) cli.printDec(matched(), 3); else cli.print("   "));
    LOG_MATCH(cli.print(' '));
    //                              0123456789012345
    static constexpr char line[] = " W A=xxxx D=xx L";
    static constexpr char STATUS[] = {' ', 'V', 'S', 'H'};
    static auto &buffer = *new CharBuffer(line);
    auto status = (cntl() & (CNTL_BA | CNTL_BS)) >> 2;
    buffer[0] = STATUS[status];
    buffer[1] = write() ? 'W' : 'R';
    buffer[15] = fetch() ? 'L' : ' ';
    buffer.hex16(5, addr);
    buffer.hex8(12, data);
    cli.println(buffer);
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

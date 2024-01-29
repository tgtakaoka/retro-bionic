#include "signals_mc6800.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc6800_config.h"

namespace debugger {
namespace mc6800 {

void Signals::getAddr() {
    addr = busRead(AL) | busRead(AM) | busRead(AH);
    vma() = digitalReadFast(PIN_VMA);
    clearFetch();
}

void Signals::getDirection() {
    rw() = digitalReadFast(PIN_RW);
}

void Signals::getData() {
    data = busRead(D);
}

void Signals::print() const {
    LOG_MATCH(cli.printDec(pos(), 3));
    LOG_MATCH(if (fetch()) cli.printDec(matched(), 3); else cli.print("   "));
    LOG_MATCH(cli.print(' '));
    static constexpr char line[] = "W A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    buffer[0] = fetch() ? 'F' : (write() ? 'W' : 'R');
    buffer.hex16(4, addr);
    if (valid()) {
        buffer[8] = ' ';
        buffer.hex8(11, data);
    } else {
        buffer[8] = 0;
    }
    cli.println(buffer);
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

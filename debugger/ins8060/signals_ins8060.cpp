#include "signals_ins8060.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_ins8060.h"

namespace debugger {
namespace ins8060 {

void Signals::getAddr() {
    const auto db = busRead(DB);
    flags() = db;
    addr = (static_cast<uint16_t>(db & 0xF) << 12) | busRead(ADM) |
           busRead(ADL);
}

void Signals::getData() {
    data = busRead(DB);
}

void Signals::outData() const {
    busWrite(DB, data);
    busMode(DB, OUTPUT);
}

void Signals::inputMode() {
    busMode(DB, INPUT);
}

void Signals::print() const {
    LOG(cli.printDec(pos(), -4));
    //                              01234567890123
    static constexpr char line[] = " R A=xxxx D=xx";
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    buffer[0] = fetch() ? 'I' : (delay() ? 'D' : (halt() ? 'H' : ' '));
    buffer[1] = read() ? 'R' : 'W';
    buffer.hex16(5, addr);
    buffer.hex8(12, data);
#ifdef PROFILE_CYCLES
    // Every status flag, for tools/cycles_ins8060.py: the mark above shows
    // only one.
    cli.print(buffer);
    cli.print(' ');
    cli.print(fetch() ? 'I' : '-');
    cli.print(delay() ? 'D' : '-');
    cli.println(halt() ? 'H' : '-');
#else
    cli.println(buffer);
#endif
}

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

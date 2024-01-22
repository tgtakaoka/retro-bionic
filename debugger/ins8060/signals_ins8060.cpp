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

void Signals::print() const {
    LOG(cli.printDec(pos(), -4));
    //                              01234567890123
    static constexpr char line[] = " R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    buffer[0] = fetch() ? 'I' : (delay() ? 'D' : (halt() ? 'H' : ' '));
    buffer[1] = read() ? 'R' : 'W';
    buffer.hex16(5, addr);
    buffer.hex8(12, data);
    cli.println(buffer);
}

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

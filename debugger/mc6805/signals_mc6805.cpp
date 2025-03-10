#include "signals_mc6805.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc6805.h"

namespace debugger {
namespace mc6805 {

void Signals::print() const {
    // cli.printDec(pos(), -4);
    //                              0123456789012
    static constexpr char line[] = "W A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    buffer[0] = fetch() ? 'L' : (write() ? 'W' : (read() ? 'R': ' '));
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace mc6805
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

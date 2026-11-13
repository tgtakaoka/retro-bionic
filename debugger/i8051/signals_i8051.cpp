#include "signals_i8051.h"
#include "char_buffer.h"
#include "debugger.h"
#include "pins_i8051.h"

namespace debugger {
namespace i8051 {

void Signals::noCycle() {
    cntl() = CNTL_PSEN | CNTL_WR;
}

void Signals::clearFetch() {
    cntl() |= CNTL_PSEN;
}

void Signals::print() const {
    //                              0123456789010
    static constexpr char line[] = "R A=xxxx D=xx";
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    buffer[0] = fetch() ? 'P' : (read() ? 'R' : (write() ? 'W' : ' '));
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
#ifdef PROFILE_CYCLES
    // The matcher's opcode fetches print as I, with the bus cycles it gave
    // them, for tools/cycles_i8051.py.
    if (matched())
        buffer[0] = 'I';
    cli.print(buffer);
    cli.print(" m=");
    cli.printlnDec(matched());
#else
    cli.println(buffer);
#endif
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

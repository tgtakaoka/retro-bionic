#include "signals_tms7000.h"
#include "char_buffer.h"
#include "debugger.h"
#include "pins_tms7000.h"

namespace debugger {
namespace tms7000 {

void Signals::print() const {
    //                              0123456789012
    static constexpr char line[] = "R A=xxxx D=xx";
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (fetch()) {
        buffer[0] = 'F';
    } else if (external()) {
        buffer[0] = write() ? 'W' : 'R';
    } else {
        buffer[0] = intack() ? 'I' : ((cntl() & CNTL_RW) ? 'r' : 'w');
    }
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
#ifdef PROFILE_CYCLES
    // How many cycles the matcher gave a marked fetch, for
    // tools/cycles_tms7000.py.
    cli.print(buffer);
    cli.print(" m=");
    cli.printlnDec(fetch() ? matched() : 0);
#else
    cli.println(buffer);
#endif
}

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

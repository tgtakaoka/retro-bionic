#include "signals_mc6809.h"
#include "char_buffer.h"
#include "debugger.h"
#include "pins_mc6809_base.h"

namespace debugger {
namespace mc6809 {

void Signals::print() const {
    LOG_MATCH(cli.printDec(pos(), 3));
    LOG_MATCH(if (fetch()) cli.printDec(matched(), 3); else cli.print("   "));
    LOG_MATCH(cli.print(' '));
    //                              0123456789012345
    static constexpr char line[] = " W A=xxxx D=xx L";
    // BA:BS: running, vector fetch, SYNC acknowledge, halt or bus grant
    static constexpr char STATUS[] = {' ', 'V', 'S', 'H'};
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    auto status = (cntl() & (CNTL_BA | CNTL_BS)) >> 2;
    buffer[0] = STATUS[status];
    buffer[1] = write() ? 'W' : 'R';
    buffer[15] = fetch() ? 'L' : ' ';
    buffer.hex16(5, addr);
    buffer.hex8(12, data);
#ifdef PROFILE_CYCLES
    constexpr auto suffix = true;
#else
    const auto suffix = Debugger.verbose();  // a bench recording's
#endif
    if (suffix) {
        // CNTL0-3, and how many cycles the matcher gave a marked fetch, for
        // tools/cycles_mc6809.py.
        cli.print(buffer);
        cli.print(" c=");
        cli.printHex(cntl(), 1);
        cli.print(" m=");
        cli.printlnDec(fetch() ? matched() : 0);
    } else {
        cli.println(buffer);
    }
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

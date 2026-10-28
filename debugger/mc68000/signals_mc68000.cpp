#include "signals_mc68000.h"
#include "char_buffer.h"
#include "debugger.h"

namespace debugger {
namespace mc68000 {

void Signals::print() const {
#ifdef PROFILE_CYCLES
    // slot, and inject/capture flags
    cli.print(readMemory() ? ' ' : 'i');
    cli.print(writeMemory() ? ' ' : 'c');
    cli.print(' ');
    cli.printDec(pos(), -4);
#endif
    //                              0123456789012345678901234567
#ifdef PROFILE_CYCLES
    // The raw strobes as the cycle opened.
    static constexpr char line[] = "R x A=xxxxxx D=xxxx c=xx";
#else
    static constexpr char line[] = "R x A=xxxxxx D=xxxx";
#endif
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (iack()) {
        buffer[0] = 'V';
    } else if (read()) {
        buffer[0] = 'R';
    } else {
        buffer[0] = 'W';
    }
    // P and D for supervisor program and data, p and d for user.
    const auto super = (fc() & 4) != 0;
    if (program()) {
        buffer[2] = super ? 'P' : 'p';
    } else if (dataSpace()) {
        buffer[2] = super ? 'D' : 'd';
    } else {
        buffer[2] = '0' + fc();
    }
    buffer.hex24(6, addr);
    if (wordAccess()) {
        buffer.hex16(15, data);
    } else {
        buffer.hex8(15, data);
        buffer[17] = buffer[18] = ' ';
    }
#ifdef PROFILE_CYCLES
    buffer.hex8(22, cntl());
#endif
    cli.println(buffer);
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

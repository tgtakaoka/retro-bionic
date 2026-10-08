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
    // The MC6809 has no VMA; CNTL0 is its XTAL pin, or AVMA on the
    // MC6809E, which tells of the next cycle. Dummy cycles read FFFF,
    // which the cycle sequences match as N.
    cntl() = busRead(CNTL) | CNTL_VMA;
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

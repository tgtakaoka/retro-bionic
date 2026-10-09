#include "signals_z8000.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_z8000.h"

namespace debugger {
namespace z8000 {

void Signals::getAddr() {
    addr = busRead(AD);
}

void Signals::getControl() {
    status() = busRead(ST);
    const auto rwbw = busRead(RWBW);
    rw() = rwbw & 1;
    bw() = (rwbw >> 1) & 1;
    ns() = digitalReadFast(PIN_NS);
}

void Signals::getData() {
    data = busRead(AD);
}

void Signals::outData() const {
    busWrite(AD, data);
    busMode(AD, OUTPUT);
}

void Signals::inputMode() const {
    busMode(AD, INPUT);
}

void Signals::print() const {
#ifdef PROFILE_CYCLES
    // slot, and inject/capture flags
    cli.print(readMemory() ? ' ' : 'i');
    cli.print(writeMemory() ? ' ' : 'c');
    cli.print(' ');
    cli.printDec(pos(), -4);
#endif
    //                              0123456789012345678901234
    static constexpr char line[] = "R A=xxxx D=xxxx S=x b=x n=x";
#ifdef PROFILE_CYCLES
    constexpr auto suffix = true;
#else
    const auto suffix = Debugger.verbose();
#endif
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (fetch()) {
        buffer[0] = 'I';
    } else if (read()) {
        buffer[0] = 'R';
    } else {
        buffer[0] = 'W';
    }
    buffer[2] = (ioReq() || specialIo()) ? 'I' : ack() ? 'V' : 'A';
    buffer.hex16(4, addr);
    if (wordAccess()) {
        buffer.hex16(11, data);
    } else if (ioReq() || (addr & 1)) {
        // A byte at an odd address, and standard I/O, ride AD0-AD7.
        buffer[11] = buffer[12] = ' ';
        buffer.hex8(13, data);
    } else {
        // A byte at an even address rides AD8-AD15.
        buffer.hex8(11, data >> 8);
        buffer[13] = buffer[14] = ' ';
    }
    if (suffix) {
        buffer.hex4(18, st());
        buffer.hex4(22, bw());
        buffer.hex4(26, ns());
    } else {
        buffer[15] = 0;
    }
    cli.println(buffer);
}

}  // namespace z8000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

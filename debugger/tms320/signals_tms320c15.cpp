#include "signals_tms320c15.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms320c15.h"

namespace debugger {
namespace tms320c15 {

void Signals::getAddr() {
    addr = busRead(AL) | busRead(AM) | busRead(AH);
#ifdef PROFILE_CYCLES
    setMatched(0);
#endif
}

bool Signals::getControl() {
    constexpr auto BUS_ACTIVE = CNTL_MEM | CNTL_WE | CNTL_DEN;
    cntl() = busRead(CNTL);
    return (cntl() ^ BUS_ACTIVE) != 0;
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void Signals::inputMode() const {
    busMode(DATA, INPUT);
}

bool Signals::fetch() const {
    return (cntl() & CNTL_MEM) == 0;
}

bool Signals::write() const {
    return (cntl() & CNTL_WE) == 0;
}

bool Signals::read() const {
    return (cntl() & CNTL_DEN) == 0;
}

void Signals::print() const {
    // cli.print(readMemory() ? ' ' : 'i');
    // cli.print(writeMemory() ? ' ' : 'c');
    // cli.print(' ');
    // cli.printDec(pos(), -4);
    //                              01234567890123
    static constexpr char line[] = "W A=xxx D=xxxx";
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (fetch()) {
        buffer[0] = 'P';
        buffer[2] = 'A';
    } else if (read()) {
        buffer[0] = 'R';
        buffer[2] = 'I';
    } else if (write()) {
        buffer[0] = 'W';
        buffer[2] = (addr >= 8) ? 'A' : 'I';
    }
    buffer.hex12(4, addr);
    buffer.hex16(10, data);
#ifdef PROFILE_CYCLES
    // The matcher's opcode fetches print as I, with the cycles it gave
    // them, for tools/cycles_tms320c15.py.
    if (matched())
        buffer[0] = 'I';
    cli.print(buffer);
    cli.print(" m=");
    cli.printlnDec(matched());
#else
    cli.println(buffer);
#endif
}

}  // namespace tms320c15
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

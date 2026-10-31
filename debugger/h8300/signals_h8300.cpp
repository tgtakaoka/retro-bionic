#include "signals_h8300.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_h8300.h"

namespace debugger {
namespace h8300 {

// A0-A7 on P20-P27, then A8-A11 and A12-A15 from P30-P33 and P34-P37.
void Signals::getAddr() {
    const auto high = busRead(AH);
    addr = busRead(AL) | (high & 0xF) << 8 | (high >> 16 & 0xF) << 12;
}

void Signals::getData() {
    data = busRead(D);
}

// #IRQ0, NMI and #WAIT are on GPIO7, TXD and RXD on GPIO6; busWriteFast()
// touches only the data bits, and A0-A7 stay inputs.
void Signals::stageData() const {
    busWriteFast(D, data);
}

void Signals::driveData() const {
    busMode(D, OUTPUT);
}

void Signals::inputMode() const {
    busMode(D, INPUT);
}

void Signals::print() const {
#ifdef PROFILE_CYCLES
    // slot, and inject/capture flags
    cli.print(readMemory() ? ' ' : 'i');
    cli.print(writeMemory() ? ' ' : 'c');
    cli.print(' ');
    cli.printDec(pos(), -4);
#endif
    //                              01234567890123456789
#ifdef PROFILE_CYCLES
    // The raw strobes as #AS opened the cycle and once they settled.
    static constexpr char line[] = "R A=xxxx D=xx c=xx c=xx";
#else
    static constexpr char line[] = "R A=xxxx D=xx";
#endif
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    buffer[0] = read() ? 'R' : 'W';
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
#ifdef PROFILE_CYCLES
    buffer.hex8(16, cntl());
    buffer.hex8(21, strobes());
#endif
    cli.println(buffer);
}

}  // namespace h8300
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

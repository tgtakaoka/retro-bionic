#include "signals_h16.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_h16.h"

namespace debugger {
namespace h16 {

// A1-A16 as A1/D0-A16/D15 carry them up to T2, and A17-A23, PF and ST
// from one GPIO7 read. A byte cycle's A0 comes later, from the strobes.
void Signals::getAddr() {
    const uint32_t ad = busRead(AD);
    const uint32_t upper = PIN(UPPER);
    addr = (busValue(ALO, upper) | busValue(AHI, upper)) << 17 | ad << 1;
    status() = busValue(ST, upper) | busValue(PF, upper);
}

void Signals::getData() {
    data = busRead(AD);
}

// #IRQ0 and #NMI share GPIO6 with the bus; busWriteFast() touches only
// the AD bits.
void Signals::stageData() const {
    busWriteFast(AD, data);
}

void Signals::driveData() const {
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
    //                              0123456789012345678901234567
#ifdef PROFILE_CYCLES
    // The raw strobes as #AS opened the cycle and with the data strobes.
    static constexpr char line[] = "R P A=xxxxxx D=xxxx c=xx c=xx";
#else
    static constexpr char line[] = "R P A=xxxxxx D=xxxx";
#endif
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (iack()) {
        buffer[0] = 'V';
    } else if (refresh()) {
        buffer[0] = 'F';
    } else if (read()) {
        buffer[0] = 'R';
    } else {
        buffer[0] = 'W';
    }
    buffer[2] = programFetch() ? 'P' : st() == ST_LOCKED ? 'L' : 'D';
    buffer.hex24(6, addr);
    // Columns 15-16 show D8-D15, the even byte; columns 17-18 D0-D7.
    if (wordAccess()) {
        buffer.hex16(15, data);
    } else if (addr & 1) {
        buffer[15] = buffer[16] = ' ';
        buffer.hex8(17, data);
    } else {
        buffer.hex8(15, data >> 8);
        buffer[17] = buffer[18] = ' ';
    }
#ifdef PROFILE_CYCLES
    buffer.hex8(22, cntl());
    buffer.hex8(27, strobes());
#endif
    cli.println(buffer);
}

}  // namespace h16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

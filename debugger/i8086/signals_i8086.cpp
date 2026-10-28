#include "signals_i8086.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_i8086.h"

namespace debugger {
namespace i8086 {

void Signals::getAddr(bool bus8) {
    addr = busRead(AH) | busRead(AD);
    cntl() = busRead(CNTL);
    st() = 0;
    mode() = bus8 ? BUS8 : 0;
}

// #BHE has turned into S7 by T2; keep T1's.
void Signals::getControl() {
    cntl() = (cntl() & BHE) | (busRead(CNTL) & ~BHE);
    st() = busRead(ST);
}

void Signals::setFetch(bool bus8) {
    mode() = bus8 ? BUS8 : 0;
    // A fetch moves a word from an even address, or the odd byte.
    cntl() = busRead(CNTL) & ~BHE;
    st() = SEG_CS;
}

void Signals::setStatus(uint8_t status) {
    static constexpr uint8_t STROBES[8] = {
            RD | WR,          // 000 interrupt acknowledge
            WR | INTA,        // 001 read I/O
            RD | INTA,        // 010 write I/O
            RD | WR | INTA,   // 011 halt
            WR | INTA | MIO,  // 100 code
            WR | INTA | MIO,  // 101 read memory
            RD | INTA | MIO,  // 110 write memory
            RD | WR | INTA,   // 111 passive
    };
    auto strobes = STROBES[status & 7];
    // The 8088's IO/#M is high for I/O.
    if (bus8())
        strobes ^= MIO;
    cntl() = (cntl() & BHE) | strobes;
}

void Signals::getSegment() {
    st() = busRead(ST);
}

void Signals::getData() {
    data = bus8() ? busRead(ADL) : busRead(AD);
}

// The 8088's AD8-AD15 are A8-A15, the CPU's outputs.
void Signals::outData() const {
    if (bus8()) {
        busWriteFast(ADL, data);
        busMode(ADL, OUTPUT);
    } else {
        busWriteFast(AD, data);
        busMode(AD, OUTPUT);
    }
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
    //                              01234567890123456789012
    static constexpr char line[] = "R A=xxxxx D=xxxx S=x E=x";
#ifdef PROFILE_CYCLES
    constexpr auto suffix = true;
#else
    const auto suffix = Debugger.verbose();
#endif
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (halt()) {
        buffer[0] = 'H';
    } else if (intAck()) {
        buffer[0] = 'V';
    } else if (code()) {
        buffer[0] = 'I';
    } else if (read()) {
        buffer[0] = coproc() ? 'r' : 'R';
    } else {
        buffer[0] = coproc() ? 'w' : 'W';
    }
    buffer[2] = memory() ? 'A' : 'P';
    buffer.hex20(4, addr);
    if (wordAccess()) {
        buffer.hex16(12, data);
    } else if (!bus8() && (addr & 1)) {
        // An odd byte rides AD8-AD15.
        buffer.hex8(12, data >> 8);
        buffer[14] = buffer[15] = ' ';
    } else {
        buffer[12] = buffer[13] = ' ';
        buffer.hex8(14, data);
    }
    if (suffix) {
        buffer.hex4(19, segment());
        buffer.hex4(23, interruptEnabled());
    } else {
        buffer[16] = 0;
    }
    cli.println(buffer);
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

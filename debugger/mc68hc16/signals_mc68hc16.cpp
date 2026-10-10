#include "signals_mc68hc16.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_mc68hc16.h"

namespace debugger {
namespace mc68hc16 {

namespace {

// 74HCS153 select to output: 15ns typical at 5V, as on the Z380 board.
// ATTENTION: unmeasured on this board.
constexpr auto asel_delay_ns = 15;

}  // namespace

// ASEL1:ASEL0 rests at 00. Stepped in gray order 00, 01, 11, 10 the muxes
// hand out A0/A16, A4/A20, A8/FC and A12/SIZ in turn, one select changing
// per step, and the last step brings it back to 00. The CPU moves only on
// our clock edges, so the address holds while the muxes are stepped.
void Signals::getAddr() {
    const uint32_t sel00 = busRead(ADDR);
    digitalWriteFast(PIN_ASEL0, HIGH);  // 01
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel01 = busRead(ADDR);
    digitalWriteFast(PIN_ASEL1, HIGH);  // 11
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel11 = busRead(ADDR);
    digitalWriteFast(PIN_ASEL0, LOW);  // 10
    delayNanoseconds(asel_delay_ns);
    const uint32_t sel10 = busRead(ADDR);
    digitalWriteFast(PIN_ASEL1, LOW);  // 00
    const auto a = compose(sel00, sel01, sel11, sel10);
    addr = a.addr;
    space() = a.fc | a.siz << 4;
}

void Signals::getData() {
    data = busRead(D);
}

// #DSACK0 and #DSACK1 share GPIO6 with the data bus; busWrite() keeps
// their bits.
void Signals::outData() const {
    busWrite(D, data);
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
    //                              0123456789012345678901234567890
#ifdef PROFILE_CYCLES
    // The raw strobes and IPIPE as #AS opened the cycle and late in it,
    // and SIZ1:SIZ0.
    static constexpr char line[] = "R P A=xxxxx D=xxxx c=xx c=xx s=x";
#else
    static constexpr char line[] = "R P A=xxxxx D=xxxx";
#endif
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (iack()) {
        buffer[0] = 'V';
    } else if (fetch()) {
        buffer[0] = 'I';
    } else if (read()) {
        buffer[0] = 'R';
    } else {
        buffer[0] = 'W';
    }
    buffer[2] = program() ? 'P' : dataSpace() ? 'D' : iack() ? 'C' : '0' + fc();
    buffer.hex20(6, addr);
    // Columns 14-15 show D8-D15, the even byte; columns 16-17 D0-D7.
    if (wordAccess()) {
        buffer.hex16(14, data);
    } else if (addr & 1) {
        buffer[14] = buffer[15] = ' ';
        buffer.hex8(16, data);
    } else {
        buffer.hex8(14, data >> 8);
        buffer[16] = buffer[17] = ' ';
    }
#ifdef PROFILE_CYCLES
    buffer.hex8(21, cntl());
    buffer.hex8(26, cntl2());
    buffer.hex4(31, siz());
#endif
    cli.println(buffer);
}

}  // namespace mc68hc16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

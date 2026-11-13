#include "signals_z380.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_z380.h"

namespace debugger {
namespace z380 {

// ASEL1:ASEL0 rests at 00. Stepped in gray order 00, 01, 11, 10 the
// muxes hand out A0/A16, A4/A20, A8/A24 and A12/A28 nibbles in turn, one
// select changing per step, and the last step brings it back to 00.
// The read right after a select write still shows the old selection, the
// next one the new, measured from the Teensy side: one discarded read of
// the port settles a step.
void Signals::getAddr() {
    uint32_t a = busRead(ADDR);
    digitalWriteFast(PIN_ASEL0, HIGH);  // 01
    PIN(ADDR);
    const uint32_t sel01 = busRead(ADDR);
    digitalWriteFast(PIN_ASEL1, HIGH);  // 11
    a |= sel01 << 4;
    PIN(ADDR);
    const uint32_t sel11 = busRead(ADDR);
    digitalWriteFast(PIN_ASEL0, LOW);  // 10
    a |= sel11 << 8;
    PIN(ADDR);
    const uint32_t sel10 = busRead(ADDR);
    digitalWriteFast(PIN_ASEL1, LOW);  // 00
    addr = a | sel10 << 12;
}

void Signals::getData() {
    data = busRead(D);
}

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
    // #M1/#MRD/#MWR/#IORD/#IOWR, #BHEN, #BLEN and the walk's mark (F an
    // instruction's first fetch, B another fetch, O data, - none).
    static constexpr char line[] = "R A=xxxxxxxx D=xxxx S=xx h=x l=x m=-";
#else
    static constexpr char line[] = "R A=xxxxxxxx D=xxxx";
#endif
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (intAck()) {
        buffer[0] = 'V';
    } else if (fetch()) {
        buffer[0] = 'I';
    } else if (read()) {
        buffer[0] = 'R';
    } else if (write()) {
        buffer[0] = 'W';
    }
    buffer[2] = (ioReq() || reti()) ? 'I' : 'A';
    buffer.hex32(4, addr);
    // Columns 15-16 show D8-D15, columns 17-18 show D0-D7.
    if (wordAccess()) {
        buffer.hex16(15, data);
    } else if (!memReq() || (addr & 1)) {
        // I/O and odd memory bytes ride D0-D7.
        buffer[15] = buffer[16] = ' ';
        buffer.hex8(17, data);
    } else {
        // Even memory bytes ride D8-D15.
        buffer.hex8(15, data >> 8);
        buffer[17] = buffer[18] = ' ';
    }
#ifdef PROFILE_CYCLES
    buffer.hex8(22, ~cntl() & NONE);  // asserted strobes
    buffer.hex4(27, (ben() & BHEN) == 0);
    buffer.hex4(31, (ben() & BLEN) == 0);
    buffer[35] = fetch() ? 'F' : isByte() ? 'B' : isOperand() ? 'O' : '-';
#endif
    cli.println(buffer);
}

}  // namespace z380
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

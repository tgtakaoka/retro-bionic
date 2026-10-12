#include "signals_z8000.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_z8000.h"

namespace debugger {
namespace z8000 {

namespace {
// For the 74AHCT157 and the level shifters after ASEL changes.
constexpr auto asel_delay_ns = 15;
}  // namespace

void Signals::getAddr() {
    addr = busRead(AD);
}

// ASEL rests high, on SN. SN0-SN3 shift up to A16-A19; SN4-SN6 shift out.
void Signals::getSegAddr() {
    const uint32_t sn = busRead(SN);
    addr = ((sn << 12) | sn) & 0x7F0000 | busRead(AD);
}

void Signals::getStatus(uint32_t gpio7, uint8_t mode) {
    status() = busValue(ST, gpio7);
    this->mode() = busValue(NS, gpio7) | mode;
}

void Signals::getControl() {
    getStatus(PIN(ST), NONSEG);
    rwbw() = busRead(RWBW);
}

// The Z8001 board shows ST while ASEL is low.
void Signals::getSegControl() {
    digitalWriteFast(PIN_ASEL, LOW);
    delayNanoseconds(asel_delay_ns);
    const uint32_t gpio7 = PIN(ST);
    digitalWriteFast(PIN_ASEL, HIGH);
    getStatus(gpio7, 0);
    rwbw() = busRead(RWBW);
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
    //                              0123456789012345678901234567
    static constexpr char line[] = "R A=xxxx D=xxxx S=x b=x n=x";
    static constexpr char segLine[] = "R A=xx:xxxx D=xxxx S=x b=x n=x";
    // The segment shifts every column after the address.
    const auto x = segmented() ? 3 : 0;
#ifdef PROFILE_CYCLES
    constexpr auto suffix = true;
#else
    const auto suffix = Debugger.verbose();
#endif
    auto &buffer = Cycles::buffer();
    buffer.set(segmented() ? segLine : line);
    if (fetch()) {
        buffer[0] = 'I';
    } else if (read()) {
        buffer[0] = 'R';
    } else {
        buffer[0] = 'W';
    }
    buffer[2] = (ioReq() || specialIo()) ? 'I' : ack() ? 'V' : 'A';
    if (segmented()) {
        buffer.hex8(4, addr >> 16);
        buffer.hex16(7, addr);
    } else {
        buffer.hex16(4, addr);
    }
    if (wordAccess()) {
        buffer.hex16(11 + x, data);
    } else if (ioReq() || (addr & 1)) {
        // A byte at an odd address, and standard I/O, ride AD0-AD7.
        buffer[11 + x] = buffer[12 + x] = ' ';
        buffer.hex8(13 + x, data);
    } else {
        // A byte at an even address rides AD8-AD15.
        buffer.hex8(11 + x, data >> 8);
        buffer[13 + x] = buffer[14 + x] = ' ';
    }
    if (suffix) {
        buffer.hex4(18 + x, st());
        buffer.hex4(22 + x, bw());
        buffer.hex4(26 + x, ns());
    } else {
        buffer[15 + x] = 0;
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

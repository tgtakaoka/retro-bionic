#include "signals_ns32016.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_ns32016.h"

namespace debugger {
namespace ns32016 {

namespace {
// For the 74AHCT157 and the level shifters after ASEL changes.
constexpr auto asel_delay_ns = 15;
}  // namespace

void Signals::getAddr(uint32_t gpio6) {
    addr = busValue(AD, gpio6);
}

// ASEL rests low, on A16-A19. A16-A23, #DDIN, #HBE and the status hold
// until the next T1, so they are read at leisure once #ADS has risen.
void Signals::getControl() {
    const uint32_t gpio7 = PIN(ST);
    digitalWriteFast(PIN_ASEL, HIGH);
    delayNanoseconds(asel_delay_ns);
    const uint32_t high = busRead(AH);
    digitalWriteFast(PIN_ASEL, LOW);
    addr |= (high << 20) | (busValue(AH, gpio7) << 16);
    _signals[0] = busValue(ST, gpio7);
    const auto cntl = busRead(CNTL);
    uint8_t flags = 0;
    if ((cntl & CNTL_DDIN) == 0)
        flags |= READ;
    if ((cntl & CNTL_HBE) == 0)
        flags |= HBE;
    if (GPIO6_PSR & US_bm)
        flags |= USER;
    _signals[1] = flags;
}

void Signals::copyControl(const Signals *s) {
    addr = s->addr;
    _signals[0] = s->_signals[0];
    _signals[1] = s->_signals[1];
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
    //                              01234567890123456789012
    static constexpr char line[] = "R A=xxxxxx D=xxxx S=x u";
#ifdef PROFILE_CYCLES
    constexpr auto suffix = true;
#else
    const auto suffix = Debugger.verbose();
#endif
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (fetch()) {
        buffer[0] = 'I';
    } else if (fetching()) {
        buffer[0] = 'i';
    } else if (intAck()) {
        buffer[0] = 'V';
    } else {
        buffer[0] = read() ? 'R' : 'W';
    }
    buffer.hex24(4, addr);
    if (wordAccess()) {
        buffer.hex16(13, data);
    } else if (lowLane()) {
        buffer[13] = buffer[14] = ' ';
        buffer.hex8(15, data);
    } else {
        // An odd byte rides AD8-AD15.
        buffer.hex8(13, data >> 8);
        buffer[15] = buffer[16] = ' ';
    }
    if (suffix) {
        buffer.hex4(20, st());
        buffer[22] = user() ? 'u' : 's';
    } else {
        buffer[17] = 0;
    }
    cli.println(buffer);
}

}  // namespace ns32016
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

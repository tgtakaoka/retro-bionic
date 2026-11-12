#include "signals_dcj11.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_dcj11.h"

namespace debugger {
namespace dcj11 {

namespace {
// Per select step, through the 153, the TXS0108E and the GPIO
// synchronizer.
constexpr auto mux_ns = 15;

// Three octal digits of a byte.
void oct8(CharBuffer &buffer, uint_fast8_t pos, uint8_t val) {
    buffer.oct3(pos, val >> 6);
    buffer.oct6(pos + 1, val);
}
}  // namespace

// The 153s rest at select 1 and step 1, 0, 2, one select per step; each
// sample is put away while the next settles. Back at 1 afterwards.
void Signals::getAddr() {
    const uint32_t sel1 = PIN(MUX);
    digitalWriteFast(PIN_ASEL0, LOW);  // 0
    addr = busRead(DAL);
    uint32_t v = (sel1 & MUX_gm) << 4;
    delayNanoseconds(mux_ns);
    const uint32_t sel0 = PIN(MUX);
    digitalWriteFast(PIN_ASEL1, HIGH);  // 2
    v |= sel0 & MUX_gm;
    delayNanoseconds(mux_ns);
    _signals[0] = PIN(MUX) & MUX_gm;
    digitalWriteFast(PIN_ASEL1, LOW);
    digitalWriteFast(PIN_ASEL0, HIGH);  // 1
    _signals[1] = (v >> 6) & 3;
    addr |= (v & 077) << 16;
}

void Signals::getData() {
    data = busRead(DAL);
}

void Signals::outData() const {
    busWrite(DAL, data);
    busMode(DAL, OUTPUT);
}

void Signals::inputMode() const {
    busMode(DAL, INPUT);
}

void Signals::print() const {
#ifdef PROFILE_CYCLES
    // slot, and inject/capture flags
    cli.print(readMemory() ? ' ' : 'i');
    cli.print(writeMemory() ? ' ' : 'c');
    cli.print(' ');
    cli.printDec(pos(), -4);
#endif
    //                              0123456789012345678901
    static constexpr char line[] = "R  A=xxxxxxxx D=xxxxxx";
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (iack()) {
        buffer[0] = 'V';
    } else if (gpRead()) {
        buffer[0] = 'g';
    } else if (gpWrite()) {
        buffer[0] = 'G';
    } else if (fetch()) {
        buffer[0] = request() ? 'i' : 'I';
    } else if (write()) {
        buffer[0] = 'W';
    }
    if (!iack() && !gpRead() && !gpWrite()) {
        static constexpr char BS[] = " spc";
        buffer[1] = BS[bs()];
    }
    buffer.oct24(5, addr);
    if (byteWrite()) {
        // The byte on the half the address picks.
        buffer[16] = buffer[17] = buffer[18] = ' ';
        oct8(buffer, 19, writeHigh() ? (data >> 8) : data);
    } else {
        buffer.oct18(16, data);
    }
    cli.println(buffer);
}

}  // namespace dcj11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

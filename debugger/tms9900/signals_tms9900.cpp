#include "signals_tms9900.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_tms9900.h"

namespace debugger {
namespace tms9900 {

bool Signals::memory() const {
    // CNTL_MEMEN is active low
    return (cntl() & CNTL_MEMEN) == 0;
}

bool Signals::read() const {
    // CNTL_MEMEN is active low
    // CNTL_DBIN is active high
    return (cntl() & (CNTL_MEMEN | CNTL_DBIN)) == CNTL_DBIN;
}

bool Signals::write() const {
    // CNTL_MEMEN is active low
    // CNTL_DBIN is active high
    return (cntl() & (CNTL_MEMEN | CNTL_DBIN)) == 0;
}

bool Signals::fetch() const {
    // CNTL_MEMEN is active low
    // CNTL_IAQ is active high
    return (cntl() & (CNTL_MEMEN | CNTL_IAQ)) == CNTL_IAQ;
}

void Signals::clearFetch() {
    // CNTL_MEMEN is active low
    // CNTL_IAQ is active high
    cntl() = (cntl() | CNTL_MEMEN) & ~CNTL_IAQ;
}

bool Signals::data16() const {
    return (cntl() & CNTL_16BIT) != 0;
}

void Signals::print() const {
    // cli.printDec(pos(), -4);
    //                              0123456789012345
    static constexpr char line[] = "R A=xxxx D=xxxx ";
    static auto &buffer = *new CharBuffer(line);
    if (fetch()) {
        buffer[0] = 'I';
    } else if (read()) {
        buffer[0] = 'R';
    } else if (write()) {
        buffer[0] = 'W';
    } else {
        buffer[0] = ' ';
    }
    static const char *const bst[] = {
            "SOPL",
            "AUMSL",
            "DOP",
            "WP",
            "IOP",
            "RESET",
            "WS",
            "MID",
            "SOP",
            "AUMS",
            "INTA",
            "ST",
            "IAQ",
            "IO",
            "GM",
            "HOLDA",
    };
    buffer.hex16(4, addr);
    if (data16()) {
        buffer.hex16(11, data);
    } else {
        buffer.hex8(11, data);
        buffer[13] = 0;
    }
    cli.print(buffer);
    if (data16())
        cli.print(bst[_signals[1] & 0xF]);
    cli.println();
}

}  // namespace tms9900
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

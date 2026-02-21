#include "signals_tms9900_base.h"
#include "char_buffer.h"
#include "debugger.h"
#include "pins_tms9900_base.h"

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

bool Signals::hasBst() const {
    return (cntl() & CNTL_BST) != 0;
}

void Signals::print() const {
    // cli.printDec(pos(), -4);
    // if (read()) {
    //     cli.print(readMemory() ? 'r' : 'i');
    // } else if (write()) {
    //     cli.print(writeMemory() ? 'w' : 'c');
    // } else {
    //     cli.print(' ');
    // }
    // cli.print(' ');
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
    buffer.hex16(4, addr);
    if (data16()) {
        buffer.hex16(11, data);
    } else {
        buffer.hex8(11, data);
        buffer[13] = 0;
    }
    cli.print(buffer);
    if (hasBst()) {
        static const char *const BST_NAME[] = {
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
        cli.print(BST_NAME[bst() & 0xF]);
    }
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

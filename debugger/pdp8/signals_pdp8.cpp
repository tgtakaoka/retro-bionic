#include "signals_pdp8.h"
#include "char_buffer.h"
#include "debugger.h"
#include "pins_pdp8.h"

namespace debugger {
namespace pdp8 {

bool Signals::mem() const {
    return (cntl() & CNTL_MEM) == 0;
}

bool Signals::cp() const {
    return (cntl() & CNTL_CP) == 0;
}

bool Signals::dev() const {
    return (cntl() & CNTL_DEV) == 0;
}

bool Signals::read() const {
    return (dir() & DIR_READ) == 0;
}

bool Signals::fetch() const {
    return (dir() & DIR_IFETCH) == 0;
}

bool Signals::addr15() const {
    return (cntl() & CNTL_15BIT) != 0;
}

void Signals::print() const {
    cli.printDec(pos(), -4);
    //                              01234567890123456789
    static constexpr char line[] = "W A=xxxx  D=xxxx C=X";
    static auto &buffer = *new CharBuffer(line);
    buffer[16] = 0;
    if (mem()) {
        buffer[2] = 'M';
    } else if (dev()) {
        buffer[2] = 'D';
        buffer[16] = ' ';
        buffer.oct3(19, ioc());
    } else if (cp()) {
        buffer[2] = 'P';
    } else {
        buffer[2] = 'S';
    }
    if (fetch()) {
        buffer[0] = 'I';
    } else {
        buffer[0] = read() ? 'R' : 'W';
    }
    if (addr15()) {
        buffer.oct15(4, addr);
    } else {
        buffer.oct12(4, addr);
    }
    buffer.oct12(12, data);
    cli.println(buffer);
}

}  // namespace pdp8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

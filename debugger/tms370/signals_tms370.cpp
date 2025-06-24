#include "signals_tms370.h"
#include "char_buffer.h"
#include "debugger.h"
#include "pins_tms370.h"

namespace debugger {
namespace tms370 {

bool Signals::read() const {
    return (cntl() & (CNTL_EDS | CNTL_RW)) == CNTL_RW;
}

bool Signals::write() const {
    return (cntl() & (CNTL_EDS | CNTL_RW)) == 0;
}

bool Signals::fetch() const {
    return (cntl() & (CNTL_EDS | CNTL_OCF)) == 0;
}

void Signals::print() const {
    //                              0123456789012
    static constexpr char line[] = "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (fetch()) {
        buffer[0] = 'F';
    } else if (read()) {
        buffer[0] = 'R';
    } else {
        buffer[0] = 'W';
    }
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace tms370
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

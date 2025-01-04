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

void Signals::print() const {
    // cli.printDec(pos(), -4);
    //                              012345678901234567890
    static constexpr char line[] = "W A=xxxx  D=xxxx C=CS";
    static auto &buffer = *new CharBuffer(line);
    if (mem()) {
        buffer[2] = 'M';
        buffer[16] = 0;
    } else if (dev()) {
        buffer[2] = 'D';
        buffer[16] = ' ';
        static constexpr char IOC[] = {
                //      C0 C1 C2
                'V',  // L L L Vector;  PC=DEV
                'R',  // L L H Read;    AC=DEV
                'B',  // L H L Branch;  PC+=DEV
                'D',  // L H H Deposit; DEV=AC, AC=0
                'V',  // H L L Vector
                'O',  // H L H Or-ed;   AC|=DEV
                'B',  // H H L Beanch
                'W',  // H H H Write;   DEV=AC
        };
        buffer[19] = IOC[ioc() & 7];
        buffer[20] = (ioc() & IOC_SKIP) == 0 ? 'S' : 0;
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
    buffer.oct12(4, addr);
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

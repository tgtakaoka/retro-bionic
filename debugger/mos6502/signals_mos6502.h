#ifndef __SIGNALS_MOS6502_H__
#define __SIGNALS_MOS6502_H__

#include "signals.h"

namespace debugger {
namespace mos6502 {

enum HardwareType : uint8_t;

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getAddr24();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const;

    bool read() const;
    bool write() const;
    bool fetch() const;
    bool vector() const;
    bool addr24() const;

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace mos6502
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

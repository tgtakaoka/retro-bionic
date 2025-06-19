#ifndef __SIGNALS_I8096_H__
#define __SIGNALS_I8096_H__

#include "signals.h"

#define LOG_MATCH(e)
// #define LOG_MATCH(e) e

namespace debugger {
namespace i8096 {
struct SignalsI8096 : SignalsBase<SignalsI8096> {
    bool getAddrValid() const;
    void getAddr();
    void getData();
    void outData() const;
    void inputMode() const;

    void print() const;
    bool fetch() const;  // Instruction fetch
    bool read() const;   // Read
    bool write() const;  // Write

    void markFetch();
    void clearMark();
    void markOperand() { mark() = 1; }
    bool isOperand() const { return mark(); }

protected:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
    uint8_t mark() const { return _signals[1]; }
    uint8_t& mark() { return _signals[1]; }
};
}  // namespace i8096
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

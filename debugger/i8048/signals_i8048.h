#ifndef __SIGNALS_I8048_H__
#define __SIGNALS_I8048_H__

#include "signals.h"

namespace debugger {
namespace i8048 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    bool getControl();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const;

    bool read() const;
    bool write() const;
    bool fetch() const;
    bool port() const;
    bool valid() const;
    void markInvalid();
    void clearFetch();

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace i8048
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

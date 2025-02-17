#ifndef __SIGNALS_CDP1802_H__
#define __SIGNALS_CDP1802_H__

#include "signals.h"

namespace debugger {
namespace cdp1802 {

struct Signals final : SignalsBase<Signals> {
    void getStatus();
    void getAddr1();
    void getAddr2();
    void getDirection();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const;

    uint_fast8_t getIoAddr();
    uint_fast8_t ioAddr() const { return nBus(); }

    enum State : uint8_t {
        S_FETCH = 0,
        S_EXEC = 1,
        S_DMA = 2,
        S_INT = 3,
    };
    State sc() const;

    bool read() const;
    bool write() const;
    bool fetch() const;
    bool vector() const;

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
    uint8_t nBus() const { return _signals[1]; }
    uint8_t &nBus() { return _signals[1]; }
};

}  // namespace cdp1802
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __SIGNALS_INS8070_H__
#define __SIGNALS_INS8070_H__

#include "signals.h"

//#define LOG_MATCH(e) e
#define LOG_MATCH(e)

namespace debugger {
namespace ins8070 {

struct Signals final : SignalsBase<Signals> {
    bool getDirection();
    void getAddr();
    void getData();
    void print() const;

    bool read() const;
    bool write() const;
    bool fetch() const;
    void markFetch(bool fetch) { _signals[1] = fetch; }
    bool fetchMark() const { return _signals[1]; };

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};
}  // namespace ins8070
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

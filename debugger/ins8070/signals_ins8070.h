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

    bool read() const { return rds() == 0; }
    bool write() const { return wds() == 0; }
    bool fetch() const;
    void markFetch(bool fetch) { _signals[2] = fetch; }
    bool fetchMark() const { return _signals[2]; };

private:
    uint8_t rds() const { return _signals[0]; }
    uint8_t wds() const { return _signals[1]; }


    uint8_t &rds() { return _signals[0]; }
    uint8_t &wds() { return _signals[1]; }
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

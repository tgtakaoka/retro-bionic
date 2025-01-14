#ifndef __SIGNALS_INS8070_H__
#define __SIGNALS_INS8070_H__

#include "sigs.h"

// #define LOG_MATCH(e) e
#define LOG_MATCH(e)

namespace debugger {
namespace ins8070 {

struct Signals final : Sigs<Signals> {
    bool getDirection();
    void getAddr();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const override;

    bool read() const;
    bool write() const;
    void markFetch(bool fetch) { _fetch = fetch; }
    bool fetch() const override { return _fetch; };

private:
    uint8_t _cntl;
    uint8_t _fetch;
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

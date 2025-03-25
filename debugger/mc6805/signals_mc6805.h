#ifndef __SIGNALS_MC6805_H__
#define __SIGNALS_MC6805_H__

#include "signals.h"

#define LOG_MATCH(e) e
// #define LOG_MATCH(e)

#define CNTL_READ 1  /* CNTL0 */
#define CNTL_WRITE 2 /* CNTL1 */
#define CNTL_FETCH 4 /* CNTL2 */

namespace debugger {
namespace mc6805 {

struct Signals : SignalsBase<Signals> {
    void print() const;

    bool write() const { return (cntl() & CNTL_WRITE) == 0; }
    bool read() const { return (cntl() & CNTL_READ) == 0; }
    bool fetch() const { return (cntl() & CNTL_FETCH) != 0; }

    // for MC68HC08
    void clearFetch() { cntl() &= ~CNTL_FETCH; }
    void markFetch() { cntl() |= CNTL_FETCH; }

protected:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace mc6805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

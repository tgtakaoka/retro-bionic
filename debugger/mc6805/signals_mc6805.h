#ifndef __SIGNALS_MC6805_H__
#define __SIGNALS_MC6805_H__

#include "signals.h"

// #define LOG(e) e
#define LOG(e)

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

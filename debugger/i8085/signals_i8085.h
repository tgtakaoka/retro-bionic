#ifndef __SIGNALS_I8085_H__
#define __SIGNALS_I8085_H__

#include "signals.h"

namespace debugger {
namespace i8085 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    void getDirection();
    void getData();
    void print() const;
    void setAddress(uint16_t _addr) { addr = _addr; }

    bool read() const { return rd() == 0; }
    bool write() const { return wr() == 0; }
    bool memory() const { return iom() == 0; }
    bool fetch() const { return iom() == 0 && s() == S_FETCH; }
    bool vector() const { return inta() == 0; }

private:
    enum Status : uint8_t {
        S_HALT = 0,   // HALT
        S_WRITE = 1,  // MW or IOW
        S_READ = 2,   // MR or IOR
        S_FETCH = 3,  // OF or INA
    };

    Status s() const { return static_cast<Status>(_signals[0]); }
    uint8_t iom() const { return _signals[1]; }
    uint8_t rd() const { return _signals[2]; }
    uint8_t wr() const { return _signals[3]; }
    uint8_t inta() const { return _signals[4]; }

    uint8_t &s() { return _signals[0]; }
    uint8_t &iom() { return _signals[1]; }
    uint8_t &rd() { return _signals[2]; }
    uint8_t &wr() { return _signals[3]; }
    uint8_t &inta() { return _signals[4]; }
};

}  // namespace i8085
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

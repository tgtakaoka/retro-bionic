#ifndef __SIGNALS_I8085_H__
#define __SIGNALS_I8085_H__

#include "signals.h"

namespace debugger {
namespace i8085 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    void getStatus();
    void getDirection();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const;

    bool read() const;
    bool write() const;
    bool fetch() const;
    bool memory() const { return iom() == 0; }
    bool readEnable() const;
    bool writeEnable() const;
    bool intAck() const;

private:
    enum Status : uint8_t {
        S_HALT = 0,   // HALT
        S_WRITE = 1,  // MW or IOW
        S_READ = 2,   // MR or IOR
        S_FETCH = 3,  // OF or INA
    };

    uint8_t cntl() const { return _signals[0]; }
    uint8_t iom() const { return _signals[1]; }

    uint8_t &cntl() { return _signals[0]; }
    uint8_t &iom() { return _signals[1]; }
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

#ifndef __SIGNALS_I8085_H__
#define __SIGNALS_I8085_H__

#include "sigs.h"

namespace debugger {
namespace i8085 {

struct Signals final : Sigs<Signals> {
    void getAddress();
    void getDirection();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const override;
    void setAddress(uint16_t _addr) { addr = _addr; }

    bool read() const;
    bool write() const;
    bool memory() const;
    bool fetch() const override;
    bool vector() const;

private:
    enum Status : uint8_t {
        S_HALT = 0,   // HALT
        S_WRITE = 1,  // MW or IOW
        S_READ = 2,   // MR or IOR
        S_FETCH = 3,  // OF or INA
    };
    uint8_t _cntl;
    uint8_t _iom;
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

#ifndef __SIGNALS_I8080_H__
#define __SIGNALS_I8080_H__

#include "signals.h"

namespace debugger {
namespace i8080 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    void getStatus();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const;

    bool memory() const;
    bool read() const;
    bool write() const;
    bool ioRead() const;
    bool ioWrite() const;
    bool intAck() const;
    bool fetch() const;

private:
    enum Status : uint8_t {
        S_INTA = 0x01,
        S_READ = 0x02,
        S_STACK = 0x04,
        S_HLTA = 0x08,
        S_OUT = 0x10,
        S_M1 = 0x20,
        S_INP = 0x40,
        S_MEMR = 0x80,
    };

    uint8_t status() const { return _signals[0]; }
    uint8_t &status() { return _signals[0]; }
};

}  // namespace i8080
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

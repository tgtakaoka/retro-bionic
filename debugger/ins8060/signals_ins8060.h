#ifndef __SIGNALS_INS8060_H__
#define __SIGNALS_INS8060_H__

#include "signals.h"

#define LOG(e) e
//#define LOG(e)

namespace debugger {
namespace ins8060 {

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getData();

    void print() const;

    bool read() const { return (flags() & F_READ) != 0; }
    bool write() const { return (flags() & F_READ) == 0; }
    bool fetch() const { return (flags() & F_INST) != 0; }
    bool delay() const { return (flags() & F_DELAY) != 0; }
    bool halt() const { return (flags() & F_HALT) != 0; }

private:
    enum Flags : uint8_t {
        F_READ = 0x10,
        F_INST = 0x20,
        F_DELAY = 0x40,
        F_HALT = 0x80,
    };
    uint8_t flags() const { return _signals[0]; }
    uint8_t &flags() { return _signals[0]; }
};
}  // namespace ins8060
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

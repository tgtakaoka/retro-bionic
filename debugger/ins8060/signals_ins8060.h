#ifndef __SIGNALS_INS8060_H__
#define __SIGNALS_INS8060_H__

#include "sigs.h"

//#define LOG(e) e
#define LOG(e)

namespace debugger {
namespace ins8060 {

struct Signals final : Sigs<Signals> {
    void getAddr();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const override;

    bool read() const { return (_flags & F_READ) != 0; }
    bool write() const { return (_flags & F_READ) == 0; }
    bool fetch() const override { return (_flags & F_INST) != 0; }
    bool delay() const { return (_flags & F_DELAY) != 0; }
    bool halt() const { return (_flags & F_HALT) != 0; }

private:
    enum Flags : uint8_t {
        F_READ = 0x10,
        F_INST = 0x20,
        F_DELAY = 0x40,
        F_HALT = 0x80,
    };
    uint8_t _flags;
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

#ifndef __SIGNALS_CDP1802_H__
#define __SIGNALS_CDP1802_H__

#include "sigs.h"

namespace debugger {
namespace cdp1802 {

struct Signals final : Sigs<Signals> {
    void getStatus();
    void getAddr1();
    void getAddr2();
    void getDirection();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const override;

    bool read() const;
    bool write() const;
    bool fetch() const override;
    bool vector() const;

private:
    uint8_t _cntl;
    uint8_t sc() const;
};

}  // namespace cdp1802
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

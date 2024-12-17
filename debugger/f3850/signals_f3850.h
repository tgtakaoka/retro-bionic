#ifndef __SIGNALS_F3850_H__
#define __SIGNALS_F3850_H__

#include "signals.h"

namespace debugger {
namespace f3850 {

struct Signals final : SignalsBase<Signals> {
    Signals *getRomc();
    Signals *getData();
    void outData() const;
    static void inputMode();
    void print() const;

    bool fetch() const { return flags() & FETCH; }
    bool read() const { return flags() & READ; }
    bool write() const { return flags() & WRITE; }
    void markFetch() { flags() |= FETCH; }
    void markRead(uint16_t addr);
    void markWrite(uint16_t addr);
    void markIoRead(uint8_t addr);
    void markIoWrite(uint8_t addr);

    uint8_t romc() const { return _signals[0]; }
    uint8_t &romc() { return _signals[0]; }

private:
    static constexpr uint8_t READ = 1;
    static constexpr uint8_t WRITE = 2;
    static constexpr uint8_t FETCH = 4;
    static constexpr uint8_t IO = 8;
    uint8_t flags() const { return _signals[1]; }
    uint8_t &flags() { return _signals[1]; }
};

}  // namespace f3850
}  // namespace debugger
#endif /* __SIGNALS_F3850_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __SIGNALS_F3850_H__
#define __SIGNALS_F3850_H__

#include "sigs.h"

namespace debugger {
namespace f3850 {

struct Signals final : Sigs<Signals> {
    Signals *getRomc();
    uint8_t romc() const { return _romc; }
    Signals *getData();
    void outData() const;
    static void inputMode();

    void clear() override;
    void print() const override;

    bool fetch() const override { return _flags & FETCH; }
    bool read() const { return _flags & READ; }
    bool write() const { return _flags & WRITE; }
    void markFetch() { _flags |= FETCH; }
    void markRead(uint16_t addr);
    void markWrite(uint16_t addr);
    void markIoRead(uint8_t addr);
    void markIoWrite(uint8_t addr);

private:
    static constexpr uint8_t READ = 1;
    static constexpr uint8_t WRITE = 2;
    static constexpr uint8_t FETCH = 4;
    static constexpr uint8_t IO = 8;
    uint8_t _flags;
    uint8_t _romc;
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

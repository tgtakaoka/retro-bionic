#ifndef __DEVS_PDP11_H__
#define __DEVS_PDP11_H__

#include "devs.h"
#include "dl11.h"
#include "kw11l.h"

// The console's address.
#define DL11_BASE 0177560
// The line clock's address.
#define KW11L_BASE 0177546

namespace debugger {
namespace pdp11 {

struct DevsPdp11 final : Devs {
    // |clock|: the board has the line clock.
    DevsPdp11(bool clock = false);
    ~DevsPdp11();

    void begin() override;
    void reset() override;
    bool loopDevices() override;
    // A byte's address selects its word.
    bool isSelected(uint32_t addr) const override {
        return _dl11->isSelected(addr & ~1) || _kw11l->isSelected(addr & ~1);
    }
    uint16_t read(uint32_t addr) const override;
    // What read() would return, without its side effects.
    uint16_t peek(uint32_t addr) const;
    void write(uint32_t addr, uint16_t data) const override;
    // The vector an interrupt acknowledge reads; it drops that request.
    uint16_t vector() const override;
    // The CPU took the line clock's interrupt.
    void acknowledgeClock() const { _kw11l->acknowledge(); }

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

private:
    Dl11 *_dl11;
    Kw11l *_kw11l;
    const bool _clock;

    Device *select(uint32_t addr) const {
        return _kw11l->isSelected(addr & ~1) ? static_cast<Device *>(_kw11l)
                                             : _dl11;
    }
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __DEVS_PDP11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __DEVS_H8300_H__
#define __DEVS_H8300_H__

#include "devs.h"
#include "serial_handler.h"

namespace debugger {
namespace h8300 {

// The MC6850 ACIA in H'FF80-H'FF8F, the external gap that #IOS also
// selects, below the on-chip register field.
constexpr uint32_t ACIA_BASE = 0xFF80;

struct DevsH8300 final : Devs {
    DevsH8300();
    ~DevsH8300();

    void begin() override;
    void reset() override;
    void loop() override;
    void setIdle(bool idle) override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

    // The SCI is clocked by ø, which runs free: its handler keeps time on
    // the cycle counter.
#if defined(ENABLE_SERIAL_HANDLER)
    void sciClock(uint32_t hz) const { _sci->setClock(hz); }
#else
    void sciClock(uint32_t) const {}
#endif

private:
    Device *_acia;
#if defined(ENABLE_SERIAL_HANDLER)
    SerialHandler *_sci;
#endif
};

}  // namespace h8300
}  // namespace debugger
#endif /* __DEVS_H8300_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

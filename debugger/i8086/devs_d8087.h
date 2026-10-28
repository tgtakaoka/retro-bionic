#ifndef __DEVS_D8087_H__
#define __DEVS_D8087_H__

#include "devs_i8086.h"
#include "device.h"
#include "inst_i8086.h"

namespace debugger {
namespace i8086 {

// The 8087's INT as an interrupt request while it is high: type 2 raises
// NMI, which stops the run in the debugger; any other type raises INTR.
// Disabled at power-up; I NPX toggles it.
struct NpxInt final : Device {
    const char *name() const override { return "NPX"; }
    const char *description() const override { return "8087 INT"; }
    void print() const override;

    void reset() override { _asserted = false; }
    void loop() override;
    void enable(bool enabled) override;
    void setVector(uint8_t type, uint8_t) override;
    // The INTR type pending, 0 if none.
    uint16_t vector() const override;

private:
    uint8_t _type = InstI8086::VEC_NMI;
    bool _asserted = false;

    void negate();
};

struct DevsD8087 final : DevsI8080 {
    DevsD8087();
    ~DevsD8087();

    void reset() override;
    void loop() override;
    uint16_t vector() const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

private:
    NpxInt *_npx;
};

}  // namespace i8086
}  // namespace debugger
#endif /* __DEVS_D8087_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

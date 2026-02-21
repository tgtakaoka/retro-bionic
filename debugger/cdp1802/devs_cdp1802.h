#ifndef __DEVS_CDP1802_H__
#define __DEVS_CDP1802_H__

#include "devs.h"

#define ACIA_BASE 4

namespace debugger {
namespace cdp1802 {

struct DevsCdp1802 final : Devs {
    DevsCdp1802();
    ~DevsCdp1802();

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

    void sciLoop() const {
#if defined(ENABLE_SERIAL_HANDLER)
        _sci->loop();
#endif
    }

private:
    Device *_acia;
#if defined(ENABLE_SERIAL_HANDLER)
    SerialHandler *_sci;
#endif
};

}  // namespace cdp1802
}  // namespace debugger
#endif /* __DEVS_CDP1802H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

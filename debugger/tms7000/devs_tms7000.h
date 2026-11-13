#ifndef __DEVS_TMS7000_H__
#define __DEVS_TMS7000_H__

#include "devs.h"
#include "mc6850.h"
#include "tms7002_serial_handler.h"

#define ACIA_BASE 0x01F0  // P240

namespace debugger {
namespace tms7000 {

enum HardwareType : uint8_t;

struct DevsTms7000 final : Devs {
    DevsTms7000();
    ~DevsTms7000();

    void begin() override;
    void reset() override;
    bool loopDevices() override;
    void setIdle(bool idle) override;
    // Inline for the bus cycles: Mc6850 is final.
    bool isSelected(uint32_t addr) const override {
        return _acia->isSelected(addr);
    }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

    void addSerialHandler(HardwareType type);
    void serialLoop() const {
#if defined(ENABLE_SERIAL_HANDLER)
        if (_serial)
            _serial->loop();
#endif
    }

private:
    Mc6850 *const _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    tms7002::Tms7002SerialHandler *_serial;
#endif
};

}  // namespace tms7000
}  // namespace debugger
#endif /* __DEVS_TMS7000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __DEVS_TMS7000_H__
#define __DEVS_TMS7000_H__

#include "devs.h"
#include "serial_handler.h"

#define ACIA_BASE 0x01F0  // P240

namespace debugger {
namespace tms7000 {

enum HardwareType : uint8_t;

struct DevsTms7000 : Devs {
    DevsTms7000();
    ~DevsTms7000();

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

    void addSerialHandler(HardwareType type);
    void serialLoop() const;

private:
    Device *_acia;
    SerialHandler *_serial;
};

}  // namespace tms7000
}  // namespace debugger
#endif /* __DEVS_TMS7000H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

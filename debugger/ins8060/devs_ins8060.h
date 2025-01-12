#ifndef __DEVS_INS8060_H__
#define __DEVS_INS8060_H__

#include "devs.h"
#include "serial_handler.h"

#define ACIA_BASE 0xDF00

namespace debugger {
namespace ins8060 {

struct DevsIns8060 final : Devs {
    DevsIns8060();
    ~DevsIns8060();

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

}  // namespace ins8060
}  // namespace debugger
#endif /* __DEVS_INS8060H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __DEVS_TMS320_H__
#define __DEVS_TMS320_H__

#include "devs.h"

#define ACIA 0x0004

namespace debugger {
namespace tms320 {

struct DevsTms320 : Devs {
    DevsTms320();
    ~DevsTms320();

    void begin() override;
    void reset() override;
    void loop() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

private:
    Device *_acia;
};

}  // namespace tms320
}  // namespace debugger
#endif /* __DEVS_TMS320H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

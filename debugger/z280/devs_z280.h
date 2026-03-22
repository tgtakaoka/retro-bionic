#ifndef __DEVS_Z280_H__
#define __DEVS_Z280_H__

#include "devs.h"

#define USART 0x40

namespace debugger {
namespace z280 {

struct DevsZ280 : Devs {
    DevsZ280();
    ~DevsZ280();

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
    Device *_usart;
};

}  // namespace z280
}  // namespace debugger
#endif /* __DEVS_Z280H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

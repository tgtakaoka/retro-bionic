#ifndef __DEVS_CP1610_H__
#define __DEVS_CP1610_H__

#include "devs.h"

#define USART_BASE 0xFFF0

namespace debugger {
namespace cp1600 {

struct DevsCp1610 : Devs {
    DevsCp1610();
    ~DevsCp1610();

    void begin() override;
    void reset() override;
    void loop() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t vector() const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

private:
    Device *_usart;
};

}  // namespace cp1600
}  // namespace debugger
#endif /* __DEVS_CP1610_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

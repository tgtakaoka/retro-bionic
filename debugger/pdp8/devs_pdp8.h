#ifndef __DEVS_PDP8_H__
#define __DEVS_PDP8_H__

#include "devs.h"

#define ACIA_IO 0070
// #define ACIA_ADDR 07770

#if defined(ACIA_IO)
/**
 * IOT ACIA_IO,0; Read status
 * IOT ACIA_IO,1; Read received data
 * IOT ACIA_IO,2; Read receive vector
 * IOT ACIA_IO,3; Read transmit vector
 * IOT ACIA_IO,4; Deposit control
 * IOT ACIA_IO,5; Deposit transmit data
 * IOT ACIA_IO,6; Deposit receive vector
 * IOT ACIA_IO,7; Deposit transmit vector
 */
#endif

namespace debugger {
namespace pdp8 {

struct DevsPdp8 final : Devs {
    DevsPdp8();
    ~DevsPdp8();

    void begin() override;
    void reset() override;
    void loop() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

    uint8_t control(uint16_t addr) const;

private:
    Device *_acia;
};

}  // namespace pdp8
}  // namespace debugger
#endif /* _DEVS_PDP8H_ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

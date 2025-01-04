#ifndef __DEVS_PDP8_H__
#define __DEVS_PDP8_H__

#include "devs.h"

#if defined(ACIA_IOT)
#define ACIA_BASE 06700
/**
 * IOT 0700; Read status
 * IOT 0701; Write control, clear AC
 * IOT 0702; Or status with AC
 * IOT 0703; Write control, preserve AC
 * IOT 0704; Read received data
 * IOT 0705; Write transmit data, clear AC
 * IOT 0706; Or received data with AC
 * IOT 0707; Write transmit data, preserve AC
 */
#else
#define ACIA_BASE 07770
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
#endif /* __DEVS_PDP8H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

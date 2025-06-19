#ifndef __DEVS_I8096_H__
#define __DEVS_I8096_H__

#include "devs.h"

#define USART 0x0200

namespace debugger {
namespace i8096 {

struct DevsI8096 : Devs {
    DevsI8096();
    ~DevsI8096();

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

}  // namespace i8096
}  // namespace debugger
#endif /* __DEVS_I8096H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __DEBUGGER_SERIAL_HANDLER_H__
#define __DEBUGGER_SERIAL_HANDLER_H__

#include "device.h"

namespace debugger {

struct SerialHandler : Device {
    SerialHandler(
            uint8_t rxd, uint8_t txd, bool invRxd = false, bool invTxd = false);
    void reset() override;
    void loop() override;

private:
    const uint8_t _rxd;
    const uint8_t _txd;
    const uint8_t _polRxd;
    const uint8_t _polTxd;
    uint16_t _prescaler;
    struct Transmitter {
        uint8_t bit;
        uint8_t data;
        uint16_t delay;
    } _tx;
    struct Receiver {
        uint8_t bit;
        uint8_t data;
        uint16_t delay;
    } _rx;

protected:
    uint16_t _divider;
    uint16_t _pre_divider;
    virtual void resetHandler() = 0;
    void txloop(Transmitter &tx);
    void rxloop(Receiver &rx);
};

}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

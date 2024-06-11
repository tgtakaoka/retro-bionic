#ifndef __DEBUGGER_SERIAL_HANDLER_H__
#define __DEBUGGER_SERIAL_HANDLER_H__

#include "device.h"

namespace debugger {

struct SerialHandler : Device {
    SerialHandler() : _serialEnabled(false) {}

    void reset() override;
    void loop() override;
    void enable(bool enabled) override { _serialEnabled = enabled; }
    bool isEnabled() const override { return _serialEnabled; }
    void setIdle(bool idle) { _enabled = _serialEnabled && !idle; }

private:
    bool _serialEnabled;
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
    virtual void assert_rxd() const = 0;
    virtual void negate_rxd() const = 0;
    virtual uint8_t signal_txd() const = 0;
    void txloop(Transmitter &tx) const;
    void rxloop(Receiver &rx) const;
};

}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

#ifndef __DEBUGGER_SERIAL_HANDLER_H__
#define __DEBUGGER_SERIAL_HANDLER_H__

#include "device.h"

namespace debugger {

// Bit-bangs a CPU's on-chip serial port against the console: drives
// its RXD and samples its TXD, paced by loop() calls.
struct SerialHandler : Device {
    SerialHandler() : _serialEnabled(false), _clockHz(0), _tickCycles(0) {}

    void reset() override;
    void loop() override;
    /** A clock that runs free at |hz|: pace() counts it on the cycle counter. */
    void setClock(uint32_t hz);
    /** One loop() per prescaled tick of the free-running clock passed since the last. */
    void pace();
    void enable(bool enabled) override { _serialEnabled = enabled; }
    bool isEnabled() const override { return _serialEnabled; }
    void setIdle(bool idle) { _enabled = _serialEnabled && !idle; }

private:
    bool _serialEnabled;
    uint16_t _prescaler;
    uint32_t _clockHz;
    uint32_t _tickCycles;
    uint32_t _lastTick;
    void setTick();
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
    uint16_t _pre_divider;
    uint16_t _tx_divider;
    uint16_t _rx_divider;
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

#include "serial_handler.h"
#include "debugger.h"
#include "pins.h"

namespace debugger {

void SerialHandler::reset() {
    negate_rxd();
    resetHandler();
    _prescaler = _pre_divider;
    _rx.bit = 0;
    _tx.bit = 0;
    setTick();
}

void SerialHandler::setClock(uint32_t hz) {
    _clockHz = hz;
    setTick();
}

void SerialHandler::setTick() {
    if (_clockHz == 0)
        return;
    _tickCycles = uint64_t(_pre_divider) * F_CPU_ACTUAL / _clockHz;
    _lastTick = ARM_DWT_CYCCNT;
}

void SerialHandler::pace() {
    if (!_enabled || _tickCycles == 0)
        return;
    const uint32_t elapsed = ARM_DWT_CYCCNT - _lastTick;
    if (elapsed < _tickCycles)
        return;
    // Keep the phase, but drop the ticks of a long absence rather than
    // burst them.
    _lastTick += (elapsed < 2 * _tickCycles) ? _tickCycles : elapsed;
    txloop(_tx);
    rxloop(_rx);
}

void SerialHandler::loop() {
    if (_enabled && --_prescaler == 0) {
        _prescaler = _pre_divider;
        txloop(_tx);
        rxloop(_rx);
    }
}

void SerialHandler::txloop(Transmitter &tx) const {
    if (tx.bit == 0) {
        if (Console.available()) {
            tx.bit = 10;  // start bit + data bits + stop bit
            tx.data = Console.read();
            tx.delay = _tx_divider;
            assert_rxd();  // start bit
        }
    } else {
        if (--tx.delay == 0) {
            if (tx.data & 1) {
                negate_rxd();  // 1
            } else {
                assert_rxd();  // 0
            }
            tx.data >>= 1;
            tx.data |= 0x80;  // stop and mark bits
            tx.delay = _tx_divider;
            --tx.bit;
        }
    }
}

void SerialHandler::rxloop(Receiver &rx) const {
    if (rx.bit == 0) {
        if (signal_txd() == 0) {
            // Pins::assert_debug();
            rx.bit = 9;  // data bits + stop bit
            rx.data = 0;
            // 1.5 bit times: the middle of the first data bit
            rx.delay = _rx_divider + (_rx_divider >> 1);
            // Pins::negate_debug();
        }
    } else {
        if (--rx.delay == 0) {
            rx.data >>= 1;
            // Pins::assert_debug();
            if (signal_txd() != 0)
                rx.data |= 0x80;
            rx.delay = _rx_divider;
            if (--rx.bit == 1) {
                Console.write(rx.data);
                rx.delay = _rx_divider >> 1;
            } else {
                // Pins::negate_debug();
            }
        }
    }
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

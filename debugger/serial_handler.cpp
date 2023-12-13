#include <Arduino.h>

#include "config_debugger.h"
#include "serial_handler.h"

namespace debugger {

SerialHandler::SerialHandler(uint8_t rxd, uint8_t txd, bool invRxd, bool invTxd)
    : _rxd(rxd),
      _txd(txd),
      _polRxd(invRxd ? HIGH : LOW),
      _polTxd(invTxd ? HIGH : LOW) {}

void SerialHandler::reset() {
    digitalWrite(_rxd, HIGH ^ _polRxd);
    pinMode(_rxd, OUTPUT);
    pinMode(_txd, _polTxd == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
    resetHandler();
    _prescaler = _pre_divider;
    _rx.bit = 0;
    _tx.bit = 0;
}

void SerialHandler::loop() {
    if (_enabled && --_prescaler == 0) {
        _prescaler = _pre_divider;
        txloop(_tx);
        rxloop(_rx);
    }
}

void SerialHandler::txloop(Transmitter &tx) {
    if (tx.bit == 0) {
        if (Console.available()) {
            tx.bit = 10;  // start bit + data bits + stop bit
            tx.data = Console.read();
            tx.delay = _divider;
            digitalWrite(_rxd, LOW ^ _polRxd);  // start bit
        }
    } else {
        if (--tx.delay == 0) {
            digitalWrite(_rxd, (tx.data & 1) ^ _polRxd);
            tx.data >>= 1;
            tx.data |= 0x80;  // stop and mark bits
            tx.delay = _divider;
            if (--tx.bit == 0)
                digitalWrite(_rxd, HIGH ^ _polRxd);  // output mark/idle
        }
    }
}

void SerialHandler::rxloop(Receiver &rx) {
    if (rx.bit == 0) {
        if (digitalRead(_txd) == (LOW ^ _polTxd)) {
            rx.bit = 9;
            rx.data = 0;
            rx.delay = _divider + (_divider >> 1);
        }
    } else {
        if (--rx.delay == 0) {
            rx.data >>= 1;
            if (digitalRead(_txd) == (HIGH ^ _polTxd))
                rx.data |= 0x80;
            rx.delay = _divider;
            if (--rx.bit == 1) {
                Console.write(rx.data);
                // stop bit will be ignored.
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

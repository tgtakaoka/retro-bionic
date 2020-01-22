/* -*- mode: c++; c-basic-offset: 2; tab-width: 2; -*- */

#include "console.h"
#include "mc6850.h"
#include "pins.h"

//#define DEBUG_IRQ
//#define DEBUG_STATUS
//#define DEBUG_CONTROL
//#define DEBUG_READ
//#define DEBUG_WRITE

Mc6850::Mc6850(uint16_t baseAddr)
  : _baseAddr(baseAddr),
    _rxInt(Pins::getIrqMask(baseAddr)),
    _txInt(Pins::getIrqMask(baseAddr + 1)),
    _control(CDS_DIV1_gc),
    _status(TDRE_bm),
    _readFlags(0),
    _nextFlags(0),
    _txData(0),
    _rxData(0)
{}

void Mc6850::assertIrq(uint8_t intMask) {
  _status |= IRQF_bm;
  Pins.assertIrq(intMask);
#ifdef DEBUG_IRQ
  if (intMask & _rxInt) Console.println(F("@@ Assert RX INT"));
  if (intMask & _txInt) Console.println(F("@@ Assert TX INT"));
#endif
}

void Mc6850::negateIrq(uint8_t intMask) {
  Pins.negateIrq(intMask);
  _status &= ~IRQF_bm;
#ifdef DEBUG_IRQ
  if (intMask & _rxInt) Console.println(F("@@ Negate RX IRQ"));
  if (intMask & _txInt) Console.println(F("@@ Negate TX IRQ"));
#endif
}

void Mc6850::loop() {
  if (Console.available() > 0) {
    _rxData = Console.read();
    if (rxRegFull())
      _nextFlags |= OVRN_bm;
    _status |= RDRF_bm;
    if (rxIntEnabled())
      assertIrq(_rxInt);
  }
  // TODO: Implement flow control
  if (Console.availableForWrite() > 0) {
    if (!txRegEmpty()) {
      Console.write(_txData);
      _status |= TDRE_bm;
      if (txIntEnabled())
        assertIrq(_txInt);
    }
  }
}

void Mc6850::write(uint8_t data, uint16_t addr) {
  if (addr == _baseAddr) {
#ifdef DEBUG_CONTROL
    Console.print(F("@@ Control 0x")); Console.println(data, HEX);
#endif
    const uint8_t delta = _control ^ data;
    _control = data;
    if (cds(delta) && masterReset()) {
      negateIrq(_txInt | _rxInt);
      _status = TDRE_bm;
      _readFlags = _nextFlags = 0;
    }
    if (tcb(delta)) {
      // TODO: flow control
      if (txIntEnabled() && txRegEmpty()) {
        assertIrq(_txInt);
      } else {
        negateIrq(_txInt);
      }
    }
    if (rieb(delta)) {
      if (rxIntEnabled() && rxRegFull()) {
        assertIrq(_rxInt);
      } else {
        negateIrq(_rxInt);
      }
    }
  } else {
    _txData = data;
    _status &= ~TDRE_bm;
    if (txIntEnabled())
      negateIrq(_txInt);
#ifdef DEBUG_WRITE
    Console.print(F("@@ Write 0x")); Console.print(data, HEX);
    Console.print(F(" 0x"));
    Console.println(_status, HEX);
#endif
  }
}

uint8_t Mc6850::read(uint16_t addr) {
  if (addr == _baseAddr) {
    _readFlags = _status & (DCD_bm | OVRN_bm);
#ifdef DEBUG_STATUS
    if (_readFlags) {
      Console.print(F("@@ Status 0x")); Console.println(_status, HEX);
    }
#endif
    return _status;
  } else {
#ifdef DEBUG_READ
    const uint8_t prev_status = _status;
#endif
    _status &= ~(RDRF_bm | _readFlags);
    _status |= _nextFlags;
    _readFlags = _nextFlags = 0;
#ifdef DEBUG_READ
    Console.print(F("@@ Read 0x")); Console.print(_rxData, HEX);
    Console.print(F(" 0x")); Console.print(prev_status, HEX);
    Console.print(F("->0x")); Console.println(_status, HEX);
#endif
    if (rxIntEnabled()) {
      if (_status & (RDRF_bm | OVRN_bm)) {
        assertIrq(_rxInt);
      } else {
        negateIrq(_rxInt);
      }
    }
    return _rxData;
  }
}
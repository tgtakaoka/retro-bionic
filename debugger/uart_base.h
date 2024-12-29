#ifndef __DEBUGGER_UART_BASE_H__
#define __DEBUGGER_UART_BASE_H__

#include "device.h"

namespace debugger {

/**
 * UART device emulator
 */
struct UartBase : Device {
    void reset() override;
    void loop() override;
    // _base_addr+0*_step: UART implementation
    // _base_addr+1*_step: UART implementation
    // _base_addr+2*_step: Rx interrupt vector
    // _base_addr+3*_step: Tx interrupt vector
    bool isSelected(uint32_t addr) const override {
        return _enabled && (addr & ~_mask) == _base_addr;
    }
    void setBaseAddr(uint32_t addr) override { _base_addr = addr & ~_mask; }
    uint32_t baseAddr() const override { return _base_addr; }
    uint16_t vector() const override { return _rxVec ? _rxVec : _txVec; }

protected:
    UartBase(uint8_t step = 1) : Device(), _step(step), _mask(step * 3) {}

    const uint8_t _step;
    const uint32_t _mask;
    uint32_t _base_addr;
    uint8_t _rxIntr;
    uint8_t _txIntr;
    uint8_t _delay;
    bool _txDone;

    virtual void resetUart() = 0;
    virtual void loopUart() = 0;
    void write(uint32_t addr, uint16_t data) override;
    uint16_t read(uint32_t addr) override;
    void assertRxIntr();
    void negateRxIntr();
    void assertTxIntr();
    void negateTxIntr();
    virtual void assertIntreq(uint8_t name);
    virtual void negateIntreq(uint8_t name);

private:
    uint8_t _rxVec;
    uint8_t _txVec;
};

}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

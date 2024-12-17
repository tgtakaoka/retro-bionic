#ifndef __DEBUGGER_I8251_H__
#define __DEBUGGER_I8251_H__

#include "uart_base.h"

namespace debugger {

/**
 * i8251 USART device emulator.
 *
 * Support only asynchronous mode. Stop bit and bit length, clock aren't
 * emulated. Interrupt support (via TxEMPTY, RxRDY).
 */
struct I8251 final : UartBase {
    I8251() : UartBase() {}

    const char *name() const override;
    const char *description() const override;
    void print() const override;

    void write(uint32_t addr, uint16_t data) override;
    uint16_t read(uint32_t addr) override;

private:
    enum ModeState : uint8_t {
        STATE_MODE = 0,
        STATE_SYNC1 = 1,
        STATE_SYNC2 = 2,
        STATE_COMMAND = 3,
    };

    ModeState _state;
    uint8_t _mode;
    uint8_t _command;
    uint8_t _status;
    uint8_t _txData;
    uint8_t _rxData;

    void resetUart() override;
    void loopUart() override;
    bool rxEnabled() const { return _command & CMD_RxEN_bm; }
    bool txEnabled() const { return _command & CMD_TxEN_bm; }
    bool rxReady() const { return _status & ST_RxRDY_bm; }
    bool txReady() const { return _status & ST_TxRDY_bm; }
    bool rtsEnabled() const { return _command & CMD_RTS_bm; }

    // Bit Definition of mode regisrer
    static constexpr int MODE_STOP_gp = 6;
    static constexpr uint8_t MODE_STOP_gm = 0xC0;
    static constexpr uint8_t MODE_SYNC_gc = (0 << MODE_STOP_gp);
    static constexpr uint8_t MODE_STOP1_gc = (1 << MODE_STOP_gp);
    static constexpr uint8_t MODE_STOP15_gc = (2 << MODE_STOP_gp);
    static constexpr uint8_t MODE_STOP2_gc = (3 << MODE_STOP_gp);
    static constexpr uint8_t MODE_EVEN_bm = 0x20;
    static constexpr uint8_t MODE_PARITY_bm = 0x10;
    static constexpr int MODE_LEN_gp = 2;
    static constexpr uint8_t MODE_LEN_gm = 0x0C;
    static constexpr uint8_t MODE_LEN5_gc = (0 << MODE_LEN_gp);
    static constexpr uint8_t MODE_LEN6_gc = (1 << MODE_LEN_gp);
    static constexpr uint8_t MODE_LEN7_gc = (2 << MODE_LEN_gp);
    static constexpr uint8_t MODE_LEN8_gc = (3 << MODE_LEN_gp);
    static constexpr int MODE_BAUD_gp = 0;
    static constexpr uint8_t MODE_BAUD_gm = 0x03;
    static constexpr uint8_t MODE_BAUD_X1 = (1 << MODE_BAUD_gp);
    static constexpr uint8_t MODE_BAUD_X16 = (2 << MODE_BAUD_gp);
    static constexpr uint8_t MODE_BAUD_X64 = (3 << MODE_BAUD_gp);
    // Bit Definition of command register
    static constexpr uint8_t CMD_EH_bm = 0x80;    // Enter hunt mode
    static constexpr uint8_t CMD_IR_bm = 0x40;    // Internal Reset
    static constexpr uint8_t CMD_RTS_bm = 0x20;   // Request To Send
    static constexpr uint8_t CMD_ER_bm = 0x10;    // Error Reset
    static constexpr uint8_t CMD_SBRK_bm = 0x08;  // Send Break
    static constexpr uint8_t CMD_RxEN_bm = 0x04;  // Receive Enable
    static constexpr uint8_t CMD_DTR_bm = 0x02;   // Data Terminal Ready
    static constexpr uint8_t CMD_TxEN_bm = 0x01;  // Transmit enable

    // Bit definition of status register
    static constexpr uint8_t ST_DSR_bm = 0x80;      // Data Set Readt
    static constexpr uint8_t ST_BRK_bm = 0x40;      // BREAK detected
    static constexpr uint8_t ST_FE_bm = 0x20;       // Framing Error
    static constexpr uint8_t ST_OE_bm = 0x10;       // Overrun Error
    static constexpr uint8_t ST_PE_bm = 0x08;       // Parity Error
    static constexpr uint8_t ST_TxEMPTY_bm = 0x04;  // Transmitter empty
    static constexpr uint8_t ST_RxRDY_bm = 0x02;    // Receiver ready
    static constexpr uint8_t ST_TxRDY_bm = 0x01;    // Transmitter ready
};

}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

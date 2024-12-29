#ifndef __DEBUGGER_MC6850_H__
#define __DEBUGGER_MC6850_H__

#include "uart_base.h"

namespace debugger {

/**
 *  MC6850 ACIA device emulator
 */
struct Mc6850 final : UartBase {
    Mc6850(uint8_t step = 1) : UartBase(step) {}

    const char *name() const override;
    const char *description() const override;
    void print() const override;

    void write(uint32_t addr, uint16_t data) override;
    uint16_t read(uint32_t addr) override;

private:
    uint8_t _control;
    uint8_t _status;
    uint8_t _txData;
    uint8_t _rxData;
    uint8_t _rxVec;
    uint8_t _txVec;

    // Bit Definition of control register
    static constexpr uint8_t CDS_gm =
            0x03;  // Counter Divider Select group mask
    static constexpr uint8_t CDS_DIV1_gc = 0x00;   // Counter Divider /1
    static constexpr uint8_t CDS_DIV16_gc = 0x01;  // Counter Divider /16
    static constexpr uint8_t CDS_DIV64_gc = 0x02;  // Counter Divider /64
    static constexpr uint8_t CDS_RESET_gc = 0x03;  // Master Reset
    static constexpr uint8_t TCB_gm = 0x60;  // Transmit Control group mask
    static constexpr uint8_t TCB_DI_gc =
            0x00;  // RTS=Low,  Tx Interrupt Disabled
    static constexpr uint8_t TCB_EI_gc =
            0x20;  // RTS=LOW,  Tx Interrupt Enabled
    static constexpr uint8_t TCB_RTS_gc =
            0x40;  // RTS=High, Tx Interrupt Disabled
    static constexpr uint8_t TCB_BREAK_gc =
            0x60;  // RTS=Low,  Tx Interrupt Disabled
                   // Transmit Break Level
    static constexpr uint8_t RIEB_bm =
            0x80;  // Receive Interrupt Enable bit mask

    static constexpr uint8_t cds(uint8_t control) { return control & CDS_gm; }
    static constexpr uint8_t tcb(uint8_t control) { return control & TCB_gm; }
    static constexpr uint8_t rieb(uint8_t control) { return control & RIEB_bm; }

    void resetUart() override;
    void loopUart() override;
    bool masterReset() const { return cds(_control) == CDS_RESET_gc; }
    bool txIntEnabled() const { return tcb(_control) == TCB_EI_gc; }
    bool rxIntEnabled() const { return rieb(_control); }
    bool txReady() const { return _status & TDRE_bm; }
    bool rxReady() const { return _status & RDRF_bm; }
    void assertIntreq(uint8_t name) override;
    void negateIntreq(uint8_t name) override;

    // Bit definition of status register
    // DCD, CTS, FERR, PERR are always zero.
    static constexpr uint8_t RDRF_bm = 0x01;  // Receive Data Register Full
    static constexpr uint8_t TDRE_bm = 0x02;  // Transmit Data Register Empty
    static constexpr uint8_t DCD_bm = 0x04;   // Data Carieer Detect
    static constexpr uint8_t CTS_bm = 0x08;   // Clear to Send
    static constexpr uint8_t FERR_bm = 0x10;  // Framing Error
    static constexpr uint8_t OVRN_bm = 0x20;  // Receiver Overrun
    static constexpr uint8_t PEER_bm = 0x40;  // Parity Error
    static constexpr uint8_t IRQF_bm = 0x80;  // Interrupt Request
};

}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

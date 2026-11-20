#ifndef __DL11_H__
#define __DL11_H__

#include "uart_base.h"

namespace debugger {
namespace pdp11 {

/**
 * DL11 asynchronous line interface, the console: RCSR, RBUF, XCSR and
 * XBUF, a word each. A request is raised as DONE or READY comes with its
 * interrupt enable set, or the enable is set with it, and is dropped at
 * its acknowledge: the vectors are the interrupt names.
 */
struct Dl11 final : UartBase {
    Dl11();

    const char *name() const override;
    const char *description() const override;
    void print() const override;

    void write(uint32_t addr, uint16_t data) override;
    uint16_t read(uint32_t addr) override;
    uint16_t peek(uint32_t addr) const override;

    // The vector of the request an acknowledge grants, the receiver's
    // first; 0 with none.
    uint16_t acknowledge();

private:
    bool _rxDone;
    bool _rxIe;
    bool _rxRequest;
    bool _txReady;
    bool _txIe;
    bool _txRequest;
    uint8_t _rxData;
    uint8_t _txData;
    uint32_t _rxTime = 0;  // when the last character came, in CPU cycles

    static constexpr uint8_t RCSR = 0;
    static constexpr uint8_t RBUF = 2;
    static constexpr uint8_t XCSR = 4;
    static constexpr uint8_t XBUF = 6;
    static constexpr uint16_t DONE_bm = 0200;  // RCSR DONE, XCSR READY
    static constexpr uint16_t IE_bm = 0100;    // Interrupt enable

    void resetUart() override;
    void loopUart() override;
    void negateIntreq(uint8_t name) override;
    void requestRx();
    void cancelRx();
    void requestTx();
    void cancelTx();
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __DL11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

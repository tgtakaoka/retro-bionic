#include <Arduino.h>

#include "debugger.h"
#include "i8251.h"

namespace debugger {

struct I8251 USART;

const char *I8251::name() const {
    return "USART";
}

const char *I8251::description() const {
    return "i8251";
}

void I8251::print() const {
    cli.print("  INT: Rx=");
    cli.printHex(_rxIntr, 2);
    cli.print(" Tx=");
    cli.printHex(_txIntr, 2);
    cli.println();
}

void I8251::resetUart() {
    _state = STATE_MODE;
    _mode = 0;
    _status = ST_TxRDY_bm | ST_TxEMPTY_bm;
    _command = 0;
    _txData = _rxData = 0;
}

void I8251::loopUart() {
    if (rxEnabled()) {
        if (Console.available() > 0 && rtsEnabled()) {
            _rxData = Console.read();
            if (rxReady())
                _status |= ST_OE_bm;
            _status |= ST_RxRDY_bm;
            assertRxIntr();
        }
    }
    // TODO: Implement flow command
    if (txEnabled()) {
        if (_txDone) {
            _status |= ST_TxEMPTY_bm;
            assertTxIntr();
        }
        if (Console.availableForWrite() > 0 && !txReady()) {
            Console.write(_txData);
            _status |= ST_TxRDY_bm;
            _txDone = true;     // defer Tx interrupt
        }
    }
}

void I8251::write(uint32_t addr, uint16_t data) {
    if (addr == _base_addr) {
        _txData = data;
        _status &= ~(ST_TxRDY_bm | ST_TxEMPTY_bm);
        negateTxIntr();
        return;
    }

    if (addr == _base_addr + 1U) {
        if (_state == STATE_COMMAND) {
            if (data & CMD_IR_bm)
                reset();
            if (data & CMD_ER_bm)
                _status &= ~(ST_FE_bm | ST_OE_bm | ST_PE_bm);
            _command = data &
                       (CMD_RTS_bm | CMD_RxEN_bm | CMD_DTR_bm | CMD_TxEN_bm);
            if (txEnabled() && (_status & ST_TxRDY_bm)) {
                assertTxIntr();
            } else {
                negateTxIntr();
            }
            if (rxEnabled() && (_status & ST_RxRDY_bm)) {
                assertRxIntr();
            } else {
                negateRxIntr();
            }
            return;
        }
        if (_state == STATE_MODE) {
            _mode = data;
            _state = ((data & MODE_STOP_gm) == MODE_SYNC_gc) ? STATE_SYNC1
                                                             : STATE_COMMAND;
            return;
        }
        if (_state == STATE_SYNC1) {
            _state = STATE_SYNC2;
            return;
        }
        // STATE_SYNC2
        _state = STATE_COMMAND;
        return;
    }
    UartBase::write(addr, data);
}

uint16_t I8251::read(uint32_t addr) {
    if (addr == _base_addr) {
        _status &= ~ST_RxRDY_bm;
        negateRxIntr();
        return _rxData;
    }
    if (addr == _base_addr + 1U)
        return _status;
    return UartBase::read(addr);
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

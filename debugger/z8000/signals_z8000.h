#ifndef __SIGNALS_Z8000_H__
#define __SIGNALS_Z8000_H__

#include "signals.h"

namespace debugger {
namespace z8000 {

// Status ST3-ST0 (Z8000 CPU User's Reference Manual, Table 9-1).
enum BUS_ST : uint8_t {
    ST_INTERNAL = 0x0,    // internal operation, no #DS
    ST_REFRESH = 0x1,     // memory refresh, no #DS
    ST_IO = 0x2,          // standard I/O
    ST_SPECIAL_IO = 0x3,  // special I/O
    ST_SEGT_ACK = 0x4,    // segment trap acknowledge
    ST_NMI_ACK = 0x5,     // non-maskable interrupt acknowledge
    ST_NVI_ACK = 0x6,     // non-vectored interrupt acknowledge
    ST_VI_ACK = 0x7,      // vectored interrupt acknowledge
    ST_DATA = 0x8,        // data memory
    ST_STACK = 0x9,       // stack memory
    ST_EPU_DATA = 0xA,    // data memory, EPU
    ST_EPU_STACK = 0xB,   // stack memory, EPU
    ST_PROGRAM = 0xC,     // program memory, subsequent words
    ST_FETCH = 0xD,       // instruction fetch, first word
    ST_EPU_CPU = 0xE,     // CPU-EPU transfer
    ST_RESERVED = 0xF,
};

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;

    uint8_t st() const { return _signals[0]; }
    bool memReq() const {
        const auto status = st();
        return status == ST_DATA || status == ST_STACK ||
               status == ST_PROGRAM || status == ST_FETCH;
    }
    bool ioReq() const { return st() == ST_IO; }
    bool specialIo() const { return st() == ST_SPECIAL_IO; }
    // An acknowledge reads an identifier; it has no address.
    bool ack() const {
        const auto status = st();
        return status >= ST_SEGT_ACK && status <= ST_VI_ACK;
    }
    bool nmiAck() const { return st() == ST_NMI_ACK; }
    // A maskable acknowledge, which a device answers with its vector.
    bool intAck() const { return st() == ST_NVI_ACK || st() == ST_VI_ACK; }
    // No #DS and nothing recorded: refresh and internal operation.
    bool noData() const { return st() == ST_INTERNAL || st() == ST_REFRESH; }
    bool read() const { return rw() != 0; }
    bool write() const { return rw() == 0; }
    bool byteAccess() const { return bw() != 0; }
    bool wordAccess() const { return bw() == 0; }
    bool systemMode() const { return ns() == 0; }

    // An instruction's first word; the fetch an acknowledge follows was
    // nullified and is fetched again after the interrupt.
    bool fetch() const { return st() == ST_FETCH && !nullified(); }
    void nullify() { mark() = 1; }
    void clearMark() { mark() = 0; }

private:
    uint8_t &status() { return _signals[0]; }
    uint8_t rw() const { return _signals[1]; }
    uint8_t &rw() { return _signals[1]; }
    uint8_t bw() const { return _signals[2]; }
    uint8_t &bw() { return _signals[2]; }
    uint8_t ns() const { return _signals[3]; }
    uint8_t &ns() { return _signals[3]; }
    bool nullified() const { return _signals[4] != 0; }
    uint8_t &mark() { return _signals[4]; }
};

}  // namespace z8000
}  // namespace debugger
#endif /* __SIGNALS_Z8000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

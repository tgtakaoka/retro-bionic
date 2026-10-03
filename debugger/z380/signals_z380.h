#ifndef __SIGNALS_Z380_H__
#define __SIGNALS_Z380_H__

#include "signals.h"

namespace debugger {
namespace z380 {

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    // The strobes: whether any is asserted. Then #BHEN/#BLEN, read only
    // for a transaction. Inline in pins_z380.cpp, their only user: the
    // strobes are read every clock.
    inline bool getControl();
    inline void getBen();
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;

    // The strobes as the pins read: a clear bit is an asserted strobe.
    bool memReq() const { return (cntl() & (MRD | MWR)) != (MRD | MWR); }
    // /IORD alongside /M1 is the RETI transaction reproduced on the I/O
    // bus, whose data the CPU drives.
    bool ioReq() const {
        return (cntl() & (IORD | IOWR)) != (IORD | IOWR) && (cntl() & M1);
    }
    bool intAck() const { return (cntl() & (M1 | IORD)) == IORD; }
    bool reti() const { return (cntl() & (M1 | IORD)) == 0; }
    bool read() const {
        return (cntl() & (MRD | IORD)) != (MRD | IORD) || intAck();
    }
    bool write() const { return (cntl() & (MWR | IOWR)) != (MWR | IOWR); }
    bool any() const { return cntl() != NONE; }
    // No strobe for a long time: the CPU halted (no #HALT on this board).
    void markHalt() { cntl() = NONE, mark() = HALT; }
    bool halt() const { return mark() == HALT; }
    // /BHEN and /BLEN: both for a word, one for a byte on its lane, the
    // even address on D8-D15 and the odd one on D0-D7. I/O asserts
    // neither and moves bytes on D0-D7.
    bool wordAccess() const { return memReq() && ben() == 0; }

    // What InstZ380::walk() made of this cycle.
    void markFetch() { mark() = FETCH; }
    void markByte() { mark() = BYTE; }
    void markOperand() { mark() = OPERAND; }
    void clearMark() { mark() = 0; }
    bool fetch() const { return mark() == FETCH; }
    bool isByte() const { return mark() == BYTE; }
    bool isOperand() const { return mark() == OPERAND; }

private:
    enum : uint8_t {
        M1 = 0x01,
        MRD = 0x02,
        MWR = 0x04,
        IORD = 0x08,
        IOWR = 0x10,
        NONE = 0x1F,  // all negated
    };
    // ben() bits: P8.22 and P8.23 as BEN reads them
    enum : uint8_t { BLEN = 0x01, BHEN = 0x02 };
    enum : uint8_t { FETCH = 1, BYTE = 2, OPERAND = 3, HALT = 4 };
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
    // #BLEN and #BHEN as the pins read: a set bit is a lane not enabled
    uint8_t ben() const { return _signals[1]; }
    uint8_t &ben() { return _signals[1]; }
    uint8_t mark() const { return _signals[2]; }
    uint8_t &mark() { return _signals[2]; }
};
}  // namespace z380
}  // namespace debugger
#endif /* __SIGNALS_Z380_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

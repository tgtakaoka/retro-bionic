#ifndef __SIGNALS_Z8000_H__
#define __SIGNALS_Z8000_H__

#include "signals.h"

namespace debugger {
namespace z8000 {

// Status ST3-ST0 (Z8000 CPU User's Reference Manual, Table 9-1).
enum BUS_ST : uint8_t {
    ST_INTERNAL = 0x0,
    ST_REFRESH = 0x1,
    ST_IO = 0x2,
    ST_SPECIAL_IO = 0x3,
    ST_SEGT_ACK = 0x4,
    ST_NMI_ACK = 0x5,
    ST_NVI_ACK = 0x6,
    ST_VI_ACK = 0x7,
    ST_DATA = 0x8,
    ST_STACK = 0x9,
    ST_EPU_DATA = 0xA,
    ST_EPU_STACK = 0xB,
    ST_PROGRAM = 0xC,  // subsequent words
    ST_FETCH = 0xD,    // first word
    ST_EPU_CPU = 0xE,
    ST_RESERVED = 0xF,
};

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getSegAddr();
    void getControl();
    // The Z8001's, through ASEL.
    void getSegControl();
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
    bool ack() const {
        const auto status = st();
        return status >= ST_SEGT_ACK && status <= ST_VI_ACK;
    }
    bool nmiAck() const { return st() == ST_NMI_ACK; }
    bool intAck() const { return st() == ST_NVI_ACK || st() == ST_VI_ACK; }
    bool noData() const { return st() == ST_INTERNAL || st() == ST_REFRESH; }
    bool read() const { return rw(); }
    bool write() const { return !rw(); }
    bool byteAccess() const { return bw(); }
    bool wordAccess() const { return !bw(); }
    bool systemMode() const { return !ns(); }
    // Z8001 cycles carry seg<<16|off addresses.
    bool segmented() const { return !(_signals[2] & NONSEG); }

    // An instruction's first word, unless an acknowledge nullified it.
    bool fetch() const { return st() == ST_FETCH && !nullified(); }
    void nullify() { mark() = 1; }
    void clearMark() { mark() = 0; }

private:
    static constexpr uint8_t RW = 0x1;
    static constexpr uint8_t BW = 0x2;
    // _signals[2]: N/#S, and a Z8002 cycle.
    static constexpr uint8_t NS = 0x1;
    static constexpr uint8_t NONSEG = 0x2;

    // ST and N/#S out of one GPIO7 read; |mode| adds NONSEG or not.
    void getStatus(uint32_t gpio7, uint8_t mode);

    uint8_t &status() { return _signals[0]; }
    uint8_t &rwbw() { return _signals[1]; }
    bool rw() const { return _signals[1] & RW; }
    bool bw() const { return _signals[1] & BW; }
    bool ns() const { return _signals[2] & NS; }
    uint8_t &mode() { return _signals[2]; }
    bool nullified() const { return _signals[3] != 0; }
    uint8_t &mark() { return _signals[3]; }
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

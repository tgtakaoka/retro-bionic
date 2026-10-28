#ifndef __SIGNALS_I8086_H__
#define __SIGNALS_I8086_H__

#include "signals.h"

namespace debugger {
namespace i8086 {

// S4:S3 in T2-T4: the segment register an access uses (8086 datasheet,
// Table 2).
enum Segment : uint8_t {
    SEG_ES = 0,
    SEG_SS = 1,
    SEG_CS = 2,  // code, or none
    SEG_DS = 3,
};

struct Signals final : SignalsBase<Signals> {
    // T1: the address, #BHE and M/#IO. |bus8| is the 8088's bus.
    void getAddr(bool bus8);
    // T2: the strobes, and S3-S6 on A16-A19.
    void getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;

    bool read() const { return (cntl() & RD) == 0; }
    bool write() const { return (cntl() & WR) == 0; }
    bool intAck() const { return (cntl() & INTA) == 0; }
    // ALE with no strobe: the HLT indication.
    bool halt() const {
        return (cntl() & (RD | WR | INTA)) == (RD | WR | INTA);
    }
    // M/#IO on the 8086, IO/#M on the 8088.
    bool memory() const {
        const auto mio = (cntl() & MIO) != 0;
        return bus8() ? !mio : mio;
    }
    bool io() const { return !memory(); }
    bool bus8() const { return mode() & BUS8; }
    void setBus8(bool bus8) { mode() = bus8 ? BUS8 : 0; }
    // T1's #BHE: low for an 8086 word.
    bool bhe() const { return (cntl() & BHE) == 0; }
    // An 8086 word: #BHE low at an even address.
    bool wordAccess() const {
        return !bus8() && (cntl() & BHE) == 0 && (addr & 1) == 0;
    }
    Segment segment() const { return Segment(st() & 3); }
    bool interruptEnabled() const { return (st() & 4) != 0; }  // S5
    // A memory read through CS: a fetch, or a vector read.
    bool code() const { return read() && memory() && segment() == SEG_CS; }
    bool fetch() const { return code(); }
    // The byte at |a|, from the lanes: the even byte rides AD0-AD7.
    uint8_t byteAt(uint32_t a) const {
        return (!bus8() && (a & 1)) ? data >> 8 : data;
    }
    // Whether the access moves the byte at |a|.
    bool covers(uint32_t a) const {
        return a == addr || (wordAccess() && a == addr + 1);
    }

    // Parked: the strobes asserted, and #BHE as a fetch has it.
    void setFetch(bool bus8);

    // Maximum mode: #S2-#S0, as the strobes they stand for.
    static constexpr uint8_t STATUS_INTA = 0;
    static constexpr uint8_t STATUS_HALT = 3;
    static constexpr uint8_t STATUS_CODE = 4;
    static constexpr uint8_t STATUS_PASSIVE = 7;
    void setStatus(uint8_t status);
    // T2: S3-S6 on A16-A19.
    void getSegment();
    // The 8087's own cycle: S6 is high.
    bool coproc() const { return (st() & 8) != 0; }

private:
    // _signals[0]: CNTL, active low but M/#IO.
    static constexpr uint8_t RD = 0x01;
    static constexpr uint8_t WR = 0x02;
    static constexpr uint8_t INTA = 0x04;
    static constexpr uint8_t BHE = 0x08;
    static constexpr uint8_t MIO = 0x10;
    // _signals[2]
    static constexpr uint8_t BUS8 = 0x01;

    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
    uint8_t st() const { return _signals[1]; }
    uint8_t &st() { return _signals[1]; }
    uint8_t mode() const { return _signals[2]; }
    uint8_t &mode() { return _signals[2]; }
};

}  // namespace i8086
}  // namespace debugger
#endif /* __SIGNALS_I8086_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

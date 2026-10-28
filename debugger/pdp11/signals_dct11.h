#ifndef __SIGNALS_DCT11_H__
#define __SIGNALS_DCT11_H__

#include "signals.h"

namespace debugger {
namespace dct11 {

// One transaction of the DCT11 in 16-bit static mode, read at the #RAS
// fall (DCT11 User's Guide, Chapter 2).
struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getControl();
    void getData();
    void outData() const;
    // An IACK's vector rides DAL7-DAL2; DAL15-DAL8 carry the CPU's
    // acknowledge.
    void outVector() const;
    void inputMode() const;
    void print() const;

    bool fetch() const { return sel() == SEL_FETCH; }
    bool iack() const { return sel() == SEL_IACK; }
    bool write() const { return (cntl() & RW_gm) != RW_gm; }
    bool read() const { return !write() && !iack(); }
    // A write's halves, each low R/#Wxx line.
    bool writeHigh() const { return (cntl() & WHB) == 0; }
    bool writeLow() const { return (cntl() & WLB) == 0; }
    bool byteWrite() const { return write() && writeHigh() != writeLow(); }

    // The debugger's bookkeeping, not a pin: a cycle that is not the
    // program's.
    void mark() { _signals[2] = 1; }
    void clearMark() { _signals[2] = 0; }
    bool marked() const { return _signals[2] != 0; }

private:
    // GPIO7.16-19 in one read: #RAS, #CAS, R/#WLB, R/#WHB.
    static constexpr uint8_t RAS = 0x1;
    static constexpr uint8_t CAS = 0x2;
    static constexpr uint8_t WLB = 0x4;
    static constexpr uint8_t WHB = 0x8;
    static constexpr uint8_t RW_gm = WLB | WHB;
    // SEL1 and SEL0 (DCT11 User's Guide, Table A-10).
    static constexpr uint8_t SEL_FETCH = 0x1;
    static constexpr uint8_t SEL_IACK = 0x2;

    uint8_t cntl() const { return _signals[0]; }
    uint8_t sel() const { return _signals[1]; }
};

}  // namespace dct11
}  // namespace debugger
#endif /* __SIGNALS_DCT11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

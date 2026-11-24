#ifndef __SIGNALS_DCJ11_H__
#define __SIGNALS_DCJ11_H__

#include "signals.h"

namespace debugger {
namespace dcj11 {

// One bus cycle of the DCJ11, read with the clock stopped after #ALE
// falls (DCJ11 User's Guide, Chapter 3). |addr| is the 22-bit physical
// address.
struct Signals final : SignalsBase<Signals> {
    // AIO alone for a NIO; |abort| is the CPU's own #ABORT.
    void getAddr(bool abort);
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;

    uint8_t aio() const { return _signals[0]; }
    bool nio() const { return aio() == AIO_NIO; }
    bool fetch() const { return aio() == AIO_IREQ || aio() == AIO_IDEMAND; }
    // A prefetch the CPU may discard.
    bool request() const { return aio() == AIO_IREQ; }
    bool iack() const { return aio() == AIO_IACK; }
    // The level an IACK acknowledges, 4 to 7, one of DAL3-DAL0 (DCJ11
    // User's Guide, Table 3-4).
    uint_fast8_t iackLevel() const {
        return (addr & 010) ? 7 : (addr & 004) ? 6 : (addr & 002) ? 5 : 4;
    }
    bool gpRead() const { return aio() == AIO_GPREAD; }
    bool gpWrite() const { return aio() == AIO_GPWRITE; }
    // Bus reads and writes, fetches among them.
    bool read() const { return (aio() & 010) != 0 && aio() < AIO_IACK; }
    bool write() const { return aio() == AIO_WORD || aio() == AIO_BYTE; }
    bool byteWrite() const { return aio() == AIO_BYTE; }
    bool writeHigh() const { return byteWrite() && (addr & 1) != 0; }
    bool writeLow() const { return byteWrite() && (addr & 1) == 0; }

    // BS1-BS0 of the first part (DCJ11 User's Guide, Table 2-1).
    uint8_t bs() const { return _signals[1]; }
    bool memory() const { return bs() == BS_MEMORY; }
    bool ioPage() const { return bs() == BS_IO; }
    // 17777740-17777750, the board's registers beside the CPU's CCR.
    bool system() const { return bs() == BS_SYSTEM; }
    // A register inside the DCJ11, which drives and takes its data.
    bool internal() const { return bs() == BS_INTERNAL; }
    // The address a 16-bit program sees, the I/O page at 160000.
    uint16_t addr16() const {
        return ioPage() ? (0160000 | (addr & 017777)) : addr;
    }

    // The debugger's bookkeeping, not a pin: a cycle that is not the
    // program's.
    void mark() { _signals[2] = 1; }
    void clearMark() { _signals[2] = 0; }
    bool marked() const { return _signals[2] != 0; }
    // The CPU aborted the cycle: its address and data mean nothing.
    bool aborted() const { return _signals[3] != 0; }

private:
    // AIO3-AIO0 (DCJ11 User's Guide, Table 3-1).
    static constexpr uint8_t AIO_NIO = 017;
    static constexpr uint8_t AIO_GPREAD = 016;
    static constexpr uint8_t AIO_IACK = 015;
    static constexpr uint8_t AIO_IREQ = 014;
    static constexpr uint8_t AIO_IDEMAND = 010;
    static constexpr uint8_t AIO_GPWRITE = 005;
    static constexpr uint8_t AIO_BYTE = 003;
    static constexpr uint8_t AIO_WORD = 001;
    static constexpr uint8_t BS_MEMORY = 0;
    static constexpr uint8_t BS_SYSTEM = 1;
    static constexpr uint8_t BS_IO = 2;
    static constexpr uint8_t BS_INTERNAL = 3;
};

}  // namespace dcj11
}  // namespace debugger
#endif /* __SIGNALS_DCJ11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

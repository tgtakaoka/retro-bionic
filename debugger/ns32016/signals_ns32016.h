#ifndef __SIGNALS_NS32016_H__
#define __SIGNALS_NS32016_H__

#include "signals.h"

namespace debugger {
namespace ns32016 {

// Bus status ST3-ST0 (NS32016 datasheet 3.4.2).
enum BUS_ST : uint8_t {
    ST_IDLE = 0x0,
    ST_WAIT = 0x1,  // idle in WAIT
    ST_IDLE_SLAVE = 0x3,
    ST_INTA = 0x4,
    ST_INTA_CASCADE = 0x5,
    ST_EOI = 0x6,
    ST_EOI_CASCADE = 0x7,
    ST_FETCH = 0x8,          // sequential
    ST_FETCH_NONSEQ = 0x9,   // after a branch
    ST_DATA = 0xA,
    ST_RMW = 0xB,
    ST_EA = 0xC,             // read for an effective address
    ST_SLAVE_OPERAND = 0xD,  // slave cycles strobe #SPC, not #ADS
    ST_SLAVE_STATUS = 0xE,
    ST_SLAVE_ID = 0xF,
};

struct Signals final : SignalsBase<Signals> {
    // AD0-AD15 out of a GPIO6 read taken while #ADS was low.
    void getAddr(uint32_t gpio6);
    // Status, A16-A23, #DDIN, #HBE and U/#S, all valid through the cycle.
    void getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;

    uint8_t st() const { return _signals[0]; }
    bool read() const { return _signals[1] & READ; }
    bool write() const { return !read(); }
    // Lanes: A0 low enables AD0-AD7, #HBE low AD8-AD15.
    bool lowLane() const { return (addr & 1) == 0; }
    bool highLane() const { return _signals[1] & HBE; }
    bool wordAccess() const { return lowLane() && highLane(); }
    bool user() const { return _signals[1] & USER; }

    bool fetching() const { return st() == ST_FETCH || st() == ST_FETCH_NONSEQ; }
    // A non-sequential fetch: the first after a branch, a trap or RETT.
    bool fetch() const { return st() == ST_FETCH_NONSEQ; }
    bool memory() const { return st() >= ST_FETCH && st() <= ST_EA; }
    bool intAck() const { return st() == ST_INTA || st() == ST_INTA_CASCADE; }
    bool eoi() const { return st() == ST_EOI || st() == ST_EOI_CASCADE; }

    // The pins this cycle was captured at, for a cycle resumed later.
    void copyControl(const Signals *s);

private:
    static constexpr uint8_t READ = 0x1;  // #DDIN low
    static constexpr uint8_t HBE = 0x2;   // #HBE low
    static constexpr uint8_t USER = 0x4;  // U/#S high
};

}  // namespace ns32016
}  // namespace debugger
#endif /* __SIGNALS_NS32016_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

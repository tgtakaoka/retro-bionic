#ifndef __INST_Z280_H__
#define __INST_Z280_H__

#include <stdint.h>

namespace debugger {
namespace z280 {

struct Signals;

struct InstZ280 {
    // Where opcode bytes come from: the board's memory, or a host array.
    struct Memory {
        virtual uint16_t read_byte(uint32_t addr) const = 0;
    };

    static constexpr uint8_t NOP = 0x00;
    static constexpr uint8_t HALT = 0x76;
    static constexpr uint8_t RETN_PREFIX = 0xED;
    static constexpr uint8_t RETN = 0x45;
    static constexpr uint8_t RETIL = 0x55;  // ED 55: pops MSR and PC
    // A Z-BUS fetch is a word, so RETN is injected as one.
    static constexpr uint16_t RETN_WORD = RETN_PREFIX << 8 | RETN;

    static constexpr uint8_t RET = 0xC9;
    static constexpr uint8_t JP = 0xC3;
    // A program hands control back to the monitor by writing RST 38H
    // into the restart vector and restarting to it. Shared convention
    // with the Z80 targets, so the samples work unchanged.
    static constexpr uint8_t RST38 = 0xFF;

    static constexpr uint16_t ORG_RESET = 0x0000;
    static constexpr uint16_t ORG_RST38 = 0x0038;
    // #NMI pushes PC and vectors here in interrupt modes 0, 1 and 2.
    static constexpr uint16_t ORG_NMI = 0x0066;

    // Measured: prefetch runs up to 3 words past an instruction.
    static constexpr uint_fast8_t PREFETCH_MAX = 4;
    // Longest alternative in the tables; inst_z280.awk checks it.
    static constexpr uint_fast8_t SEQUENCE_MAX = 31;

    // Match the instruction at begin->addr against the ring [begin, end):
    // marks its cycles; nexti() is where the next one starts, nextAddr()
    // what it fetches.
    bool match(Signals *begin, const Signals *end, const Memory &mems);
    // An interrupt or trap taken instead of the fetch at |expected|.
    bool matchInterrupt(Signals *begin, const Signals *end, uint32_t expected);
    // Match the ring from |begin|. A match must be followed by what it
    // expects (next address, branch target, popped address, interrupt
    // vector, or the stop |pc|), else it is no instruction and is
    // dropped. Returns the count; |atPc|: the chain ends at the PC.
    static uint_fast8_t matchAll(Signals *begin, const Signals *end,
            const Memory &mems, uint32_t pc, bool &atPc);
    // The start whose chain ends at the PC, then the longest, then the
    // earliest. Leaves the ring marked from it.
    static Signals *findFetch(Signals *begin, const Signals *end,
            const Memory &mems, uint32_t pc);
#ifndef ARDUINO
    static bool trace;  // host test: print matchAll()'s passes
#endif
    // The ring ended inside the sequence.
    bool isEnd() const { return _insufficient; }
    // Matched, but the taken branch ran out of ring: next unknown.
    bool cutShort() const { return _cutShort; }
    uint_fast8_t nexti() const { return _nexti; }
    uint32_t nextAddr() const { return _next; }

    struct Table {
        uint8_t len;
        uint8_t seq;
    };

private:
    uint_fast8_t _len;
    bool _insufficient;
    bool _cutShort;
    uint_fast8_t _nexti;
    uint32_t _next;

    const Table *get(uint32_t pc, const Memory &mems);
    bool matchSequence(Signals *begin, const Signals *end, const char *seq,
            const Memory &mems);
};

}  // namespace z280
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

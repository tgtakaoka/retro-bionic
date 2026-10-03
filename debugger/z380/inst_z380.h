#ifndef __INST_Z380_H__
#define __INST_Z380_H__

#include <stdint.h>
#include "cycles.h"

namespace debugger {
namespace z380 {

struct Signals;

// The Z380 fetches code as a stream of aligned words, up to 5 bytes ahead
// of what it runs, so no bus cycle marks an instruction. walk() follows
// that stream from an instruction start: each instruction's bytes are
// fetched, then its data cycles and the fetch at its target, if it
// transfers control, come in any order while the stream goes on ahead. An
// interrupt, NMI or trap is the PC pushed and a vector fetched, in either
// order, between two instructions. tools/walk_z380.py is the same on the
// host.
struct InstZ380 {
    // Where opcode bytes come from: the board's memory, or a host array.
    struct Memory {
        virtual uint16_t read_byte(uint32_t addr) const = 0;
    };

    static constexpr uint8_t NOP = 0x00;
    static constexpr uint8_t HALT = 0x76;
    static constexpr uint8_t RETN_PREFIX = 0xED;
    static constexpr uint8_t RETN = 0x45;

    static constexpr uint8_t JP = 0xC3;
    // A program hands control back to the monitor by writing RST 38H
    // into the restart vector and restarting to it. Shared convention
    // with the Z80 targets, so the samples work unchanged.
    static constexpr uint8_t RST38 = 0xFF;

    static constexpr uint16_t ORG_RESET = 0x0000;
    static constexpr uint16_t ORG_RST38 = 0x0038;
    // #NMI pushes PC and vectors here, with no acknowledge transaction.
    static constexpr uint16_t ORG_NMI = 0x0066;

    // The walk of the ring [begin, end) that leaves the CPU at |pc|, from
    // the earliest fetch that has one, in Extended (|xm|) and Long Word
    // (|lw|) mode. Marks its cycles: fetch() on each instruction's first
    // fetch, isOperand() on data, isByte() on the other fetches.
    bool walk(Signals *begin, const Signals *end, const Memory &mems,
            uint32_t pc, bool xm, bool lw);
#ifndef ARDUINO
    static bool trace;  // host test: print the walk's failures
#endif

    // After walk(): the cycle it starts from, and its instructions.
    Signals *start() const { return _start; }
    uint_fast16_t steps() const { return _steps; }
    uint32_t addr(uint_fast16_t step) const { return _step[step].addr; }
    // An interrupt, NMI or trap taken: no instruction at addr().
    bool interrupt(uint_fast16_t step) const { return _step[step].interrupt; }
    // Which instruction the cycle at |begin->next(i)| belongs to.
    uint_fast16_t owner(uint_fast8_t i) const { return _owner[i]; }

    static constexpr uint_fast16_t NOBODY = UINT16_MAX;
    // Ahead of the first instruction the walk confirms: shown as is.
    static constexpr uint_fast16_t LEAD = UINT16_MAX - 1;

private:
    // How an instruction moves the PC, with F_COND and F_WIDE.
    enum Flow : uint8_t {
        F_NONE = 0,
        F_ABS = 1,     // the operand's last 2 (+IB/IW) bytes
        F_REL = 2,     // the operand bytes after the opcode, from the next
        F_IND = 3,     // wherever the next fetch goes
        F_RST = 4,     // opcode & 38H
        F_STOP = 5,    // HALT, SLP: until an interrupt
        F_REPEAT = 6,  // a block repeat, see REPEATS[]
        F_TRAP = 7,    // the PC pushed, then 0000H
    };
    static constexpr uint8_t F_COND = 0x08;  // taken or not
    static constexpr uint8_t F_WIDE =
            0x10;  // a DDIR IB or IW widens the operand

    // A z380-PAGExx.txt row: its length, flow, and the index of its
    // BUS_BYTES[], or REPEATS[] for F_REPEAT. A length of 0 is no
    // instruction.
    struct Opcode final {
        uint16_t _attr;

        static constexpr Opcode create(
                uint8_t length, uint8_t flow, uint8_t index) {
            return Opcode{static_cast<uint16_t>((length << length_gp) |
                                                (flow << flow_gp) |
                                                (index << index_gp))};
        }

        uint8_t length() const { return (_attr >> length_gp) & length_gm; }
        Flow flow() const { return Flow((_attr >> flow_gp) & flow_gm); }
        bool conditional() const { return _attr & (F_COND << flow_gp); }
        bool widened() const { return _attr & (F_WIDE << flow_gp); }
        uint8_t index() const { return (_attr >> index_gp) & index_gm; }

    private:
        static constexpr int length_gp = 0;
        static constexpr int flow_gp = 3;
        static constexpr int index_gp = 8;
        static constexpr uint_fast8_t length_gm = 0x07;
        static constexpr uint_fast8_t flow_gm = 0x07;
        static constexpr uint_fast8_t index_gm = 0x1F;
    };

    // Bytes read and written on memory and I/O, a nibble each: R, W, r, w.
    struct BusBytes {
        uint16_t taken;
        uint16_t notTaken;
        uint16_t longWord;  // in Long Word mode
        uint16_t extended;  // in Extended mode

        static constexpr uint16_t AS_TAKEN = 0xFFFF;
        static constexpr uint16_t pack(uint8_t memRead, uint8_t memWrite,
                uint8_t ioRead, uint8_t ioWrite) {
            return memRead | memWrite << 4 | ioRead << 8 | ioWrite << 12;
        }
        static constexpr uint8_t get(uint16_t packed, uint_fast8_t kind) {
            return (packed >> (4 * kind)) & 0xF;
        }
    };

    // A block repeat's iteration: its transfers and their address steps.
    struct Repeat {
        uint8_t transfers;
        char kind[2];
        int8_t step[2];

        static constexpr int8_t ANY_STEP = 0x7F;
    };

    // Generated by inst_z380.awk from z380-PAGExx.txt.
    static const BusBytes BUS_BYTES[];
    static const Repeat REPEATS[];
    static const Opcode PAGE00_TABLE[256];
    static const Opcode PAGECB_TABLE[256];
    static const Opcode PAGEED_TABLE[256];
    static const Opcode PAGEEDCB_TABLE[256];
    static const Opcode PAGEDD_TABLE[256];
    static const Opcode PAGEFD_TABLE[256];
    static const Opcode PAGEDDCB_TABLE[256];
    static const Opcode PAGEFDCB_TABLE[256];

    // An instruction decoded from memory.
    struct Decoded {
        uint32_t addr;
        uint8_t length;  // DDIR and IB/IW included
        Flow flow;
        bool conditional;
        uint8_t index;         // its BUS_BYTES[] or REPEATS[]
        uint8_t extension;     // DDIR IB/IW bytes
        uint8_t ddirWord;      // DDIR: 0 none, 1 W, 2 LW
        uint8_t ddirLength;    // the DDIR's bytes in front
        uint8_t opcodeLength;  // the opcode's bytes, prefix included
    };
    // An instruction walked, or an interrupt taken.
    struct Step {
        uint32_t addr;
        bool interrupt;
        bool taken;
        uint8_t fetchAt;  // the cycle that brought its first byte
    };
    // Where a conditional was taken: on failure, walk on not taken.
    struct Backtrack {
        uint32_t pc;
        uint32_t fetch;
        uint16_t steps;
        uint8_t i;
    };
    static constexpr uint_fast16_t MAX_STEPS = 4 * Cycles::MAX_CYCLES;
    static constexpr uint_fast8_t MAX_BACKTRACKS = 64;
    // Data cycles the first instruction may leave to one before the ring.
    static constexpr uint_fast8_t LEAD_MAX = 4;

    Signals *_begin;
    uint_fast8_t _size;
    const Memory *_mems;
    uint32_t _stop;
    bool _xm;
    bool _lw;
    uint32_t _budget;
    Signals *_start;
    uint_fast16_t _steps;
    Step _step[MAX_STEPS];
    uint16_t _owner[Cycles::MAX_CYCLES];
    Backtrack _backtrack[MAX_BACKTRACKS];
    uint_fast8_t _backtracks;

    bool decode(uint32_t pc, Decoded &inst) const;
    uint32_t transferTarget(const Decoded &inst) const;
    void busBytes(const Decoded &inst, bool taken, uint8_t bytes[4]) const;
    bool walkFrom(uint_fast8_t i, uint32_t addr);
    bool interruptAt(
            uint_fast8_t &i, uint32_t &pc, uint32_t &fetch, bool anyPc = false);
    enum Result : uint8_t { FAILED, NEXT, ENDED };
    Result execute(uint_fast8_t &i, uint32_t &pc, uint32_t &fetch,
            const Decoded &inst, bool taken);
    void repeat(uint_fast8_t &i, uint32_t &fetch, const Decoded &inst);
    bool endsAt(uint32_t pc);
    void dropUnconfirmed();
    bool addStep(uint32_t addr, bool interrupt, uint_fast8_t i);
    void assign(uint_fast8_t i, bool data);
};

}  // namespace z380
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

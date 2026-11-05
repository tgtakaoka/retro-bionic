#ifndef __FETCH_MC68000_H__
#define __FETCH_MC68000_H__

#include <stdint.h>

namespace debugger {
namespace mc68000 {

// The program-space reads of a run, sorted into the opcode stream and
// PC-relative operands; FC tells both apart from data, not from each
// other. The prefetch reads ahead in address order, so a read that does
// not follow the stream is a flush, at a branch target or a handler,
// where an instruction starts; one whose run comes back to the stream
// right after was an operand.
// ATTENTION: an operand read at the stream's very end passes for a fetch,
// and a branch to a block that jumps back exactly there for an operand.
struct FetchMc68000 {
    enum Kind : uint8_t {
        OTHER,    // not a program read
        LOOSE,    // a fetch before the first known instruction start
        FLUSH,    // a fetch that starts an instruction
        FETCH,    // a fetch that follows the stream
        OPERAND,  // a PC-relative operand read
    };
    struct Cycle {
        uint32_t addr;
        uint8_t bytes;  // 2 for a word, 1 for a byte
        bool program;
    };

    // |anchored|: the first program read starts an instruction.
    static void classify(
            const Cycle *c, uint_fast8_t n, bool anchored, Kind *kind) {
        uint32_t fetched = 0;  // past the stream's last byte
        bool inStream = false;
        bool loose = false;  // no instruction start seen yet
        uint_fast8_t i = 0;
        while (i < n) {
            if (!c[i].program) {
                kind[i++] = OTHER;
                continue;
            }
            if (inStream && c[i].addr == fetched) {
                kind[i] = loose ? LOOSE : FETCH;
                fetched += c[i++].bytes;
                continue;
            }
            // The run of reads that follow one another from here.
            auto end = c[i].addr;
            auto j = i;
            for (; j < n; ++j) {
                if (!c[j].program)
                    continue;
                if (c[j].addr != end)
                    break;
                end += c[j].bytes;
            }
            const auto operand = inStream && j < n && c[j].addr == fetched;
            if (!operand) {
                loose = !inStream && !anchored;
                fetched = end;
                inStream = true;
            }
            const auto first = operand ? OPERAND : (loose ? LOOSE : FLUSH);
            const auto rest = operand ? OPERAND : (loose ? LOOSE : FETCH);
            for (auto k = i; k < j; ++k)
                kind[k] = !c[k].program ? OTHER : (k == i ? first : rest);
            i = j;
        }
    }
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __FETCH_MC68000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

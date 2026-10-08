#ifndef __DEBUGGER_MATCH_MEMORY_H__
#define __DEBUGGER_MATCH_MEMORY_H__

#include <stdint.h>

namespace debugger {

// Where a bus cycle matcher reads opcode bytes: the board's Mems, or a
// host array in a test.
// read_byte() is Mems', whose word memories return 16 bits.
struct MatchMemory {
    virtual uint16_t read_byte(uint32_t addr) const = 0;

protected:
    ~MatchMemory() = default;
};

}  // namespace debugger
#endif /* __DEBUGGER_MATCH_MEMORY_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

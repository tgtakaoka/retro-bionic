#ifndef __MEMS_Z380_H__
#define __MEMS_Z380_H__

#include "inst_z380.h"
#include "mems.h"

namespace debugger {
namespace z380 {

// The 16MB of EXTMEM is all the main memory there is: the bus decodes
// A0-A23 and the rest of the 4GB space mirrors it.
struct MemsZ380 : ExtMemory {
    MemsZ380();

    static constexpr uint32_t ADDR_MASK = MEM_SIZE - 1;

    // A plain little-endian byte memory, as the CPU sees it. A word
    // transfer to 16-bit memory (#MSIZE high) moves the aligned pair: the
    // even-address byte on D8-D15, the odd one on D0-D7.
    // One 16-bit access of the little-endian array, swapped.
    uint16_t read_zbus(uint32_t addr) const {
        return __builtin_bswap16(read_word(addr >> 1));
    }
    void write_zbus(uint32_t addr, uint16_t data) const {
        write_word(addr >> 1, __builtin_bswap16(data));
    }

    // Disassemble as the CPU decodes in its current mode.
    void setMode(bool extended, bool longWord);

    // While set, the disassembler reads code from |code|.
    void setCode(const InstZ380::Memory *code) { _code = code; }
    uint16_t get_prog(uint32_t addr) const override {
        return _code ? _code->read_byte(addr) : ExtMemory::get_prog(addr);
    }

private:
    const InstZ380::Memory *_code = nullptr;
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

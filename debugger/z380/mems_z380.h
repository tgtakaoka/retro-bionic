#ifndef __MEMS_Z380_H__
#define __MEMS_Z380_H__

#include "inst_z380.h"
#include "mems.h"

namespace debugger {
namespace z380 {

// The 16MB of EXTMEM is all the main memory there is: the bus decodes
// A0-A23 at most and the rest of the 4GB space mirrors it. The address
// mask register narrows the decoding, as on a board with less memory.
struct MemsZ380 : ExtMemory {
    MemsZ380();

    // The address lines memory decodes, as the address mask register
    // counts them; of those, EXTMEM has A0-A23.
    uint32_t busMask() const { return _busMask; }
    void setBusMask(uint32_t mask) {
        _busMask = mask;
        _addrMask = mask & (MEM_SIZE - 1);
    }
    uint32_t addrMask() const { return _addrMask; }

    uint16_t read_byte(uint32_t addr) const override {
        return ExtMemory::read_byte(addr & _addrMask);
    }
    void write_byte(uint32_t addr, uint16_t data) const override {
        ExtMemory::write_byte(addr & _addrMask, data);
    }
    uint16_t read_word(uint32_t addr) const override {
        return ExtMemory::read_word(addr & (_addrMask >> 1));
    }
    void write_word(uint32_t addr, uint16_t data) const override {
        ExtMemory::write_word(addr & (_addrMask >> 1), data);
    }

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
    uint32_t _busMask = ~UINT32_C(0);
    uint32_t _addrMask = MEM_SIZE - 1;
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

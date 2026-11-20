#ifndef __MEMS_PDP11_H__
#define __MEMS_PDP11_H__

#include "mems.h"

namespace debugger {
namespace pdp11 {

// The FPS FD and FL bits pick how FP11 instructions disassemble.
struct FpMode {
    void fpMode(bool fd, bool fl) {
        _fd = fd;
        _fl = fl;
    }

protected:
    bool _fd = true;
    bool _fl = true;
};

// Little-endian words in |MEM|; the CPU reads whole words and writes
// bytes on the half R/#WHB or R/#WLB picks.
template <typename MEM>
struct MemsPdp11 final : MEM, FpMode {
    MemsPdp11(uint32_t maxAddr = UINT16_MAX);

    uint32_t maxAddr() const override { return _maxAddr; }

    uint16_t read(uint32_t addr) const override {
        return this->read16(addr & ~1);
    }
    void write(uint32_t addr, uint16_t data) const override {
        this->write16(addr & ~1, data);
    }

    // How far disassemble() steps past |addr|, printing nothing.
    uint_fast8_t insnLength(uint32_t addr) const;

protected:
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif

private:
    const uint32_t _maxAddr;
};

// The DCT11's 64KB.
using MemsDct11 = MemsPdp11<DmaMemory>;
// The DCJ11's 22-bit space.
using MemsDcj11 = MemsPdp11<ExtMemory>;

}  // namespace pdp11
}  // namespace debugger
#endif /* __MEMS_PDP11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

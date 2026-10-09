#ifndef __MEMS_Z8000_H__
#define __MEMS_Z8000_H__

#include "mems.h"

namespace debugger {
namespace z8000 {

// A byte memory, big-endian: a word's even byte is its high one, on
// AD8-AD15. Program words are read and written whole. The Z8001's
// addresses are seg<<16|off, 128 segments of 64KB.
struct RegsZ8000;

struct MemsZ8000 : ExtMemory {
    MemsZ8000(bool segmented);
    void setRegs(const RegsZ8000 *regs) { _regs = regs; }

    uint32_t maxAddr() const override { return _maxAddr; }

    uint16_t read(uint32_t addr) const override { return read16(addr & ~1); }
    void write(uint32_t addr, uint16_t data) const override {
        write16(addr & ~1, data);
    }

protected:
#ifdef WITH_ASSEMBLER
    libasm::Assembler *assembler() const override;
#endif
#ifdef WITH_DISASSEMBLER
    libasm::Disassembler *disassembler() const override;
#endif

private:
    const bool _segmented;
    const uint32_t _maxAddr;
    const RegsZ8000 *_regs = nullptr;
    // A Z8001 program with SEG clear in its FCW is Z8002 code.
    bool nonsegmented() const;
};

}  // namespace z8000
}  // namespace debugger
#endif /* __MEMS_Z8000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:

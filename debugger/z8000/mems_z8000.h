#ifndef __MEMS_Z8000_H__
#define __MEMS_Z8000_H__

#include "mems.h"

namespace debugger {
namespace z8000 {

struct RegsZ8000;

// Big-endian; the even byte rides AD8-AD15. Z8001 addresses are
// seg<<16|off.
struct MemsZ8000 : ExtMemory {
    // A memory segment is the high half of an EXTMEM address,
    // segment<<16 | offset. A Z8001 cycle carries its own segment in the
    // address; a Z8002 has none, so for split instruction/data it
    // synthesizes three: the system (kernel and a merged program), the
    // running program's code, and its data, each a 64K segment.
    static constexpr uint32_t SEG_SIZE = 0x10000;
    enum Seg : uint8_t { SYS = 0, CODE = 1, DATA = 2, SEGS = 3 };

    MemsZ8000(bool segmented);
    void setRegs(const RegsZ8000 *regs) { _regs = regs; }

    uint32_t maxAddr() const override { return _maxAddr; }

    // The debugger's m/L/U and the disk see the system segment.
    uint16_t read(uint32_t addr) const override { return read16(addr & ~1); }
    void write(uint32_t addr, uint16_t data) const override {
        write16(addr & ~1, data);
    }

    // Turns the split routing on or off; off (the reset state) leaves
    // every access in the system segment, so main's samples are unchanged.
    void setSplit(bool on) { _split = on; }
    bool split() const { return _split; }

    // The running CPU's access in a synthesized segment; |seg| is 0 for a
    // Z8001, whose address already carries the segment.
    uint16_t readSeg(uint32_t addr, uint8_t seg) const {
        return read16(seg * SEG_SIZE + (addr & ~1));
    }
    void writeSeg(uint32_t addr, uint16_t data, uint8_t seg) const {
        write16(seg * SEG_SIZE + (addr & ~1), data);
    }
    uint16_t readByteSeg(uint32_t addr, uint8_t seg) const {
        return read_byte(seg * SEG_SIZE + addr);
    }
    void writeByteSeg(uint32_t addr, uint16_t data, uint8_t seg) const {
        write_byte(seg * SEG_SIZE + addr, data);
    }

    // The disk cache sits above the three synthesized segments, not in them.
    Area cacheArea() const override {
        return _segmented ? ExtMemory::cacheArea()
                          : extArea(SEGS * SEG_SIZE, MEM_SIZE);
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
    bool _split = false;
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
